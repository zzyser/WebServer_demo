#include "httprequest.h"

using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
    "/index","/register","/login","/welcome","/video","picture"
};

const unordered_map<string,int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html",0},{"/login.html",1}
};
void HttpRequest::Init(){
    method_=path_=version_=body_="";
    state_=REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const{
    if(header_.count("Connection")==1){
        return header_.find("Connection")->second == "keep-alive" &&version_=="1.1";
    }
    return false;
}

//解析处理
bool HttpRequest::parse(Buffer& buff){
    const char CRLF[] = "\r\n";
    if(buff.ReadableBytes() <= 0){
        return false;
    }
    //读取数据
    while(buff.ReadableBytes() && state_ != FINISH){
        //从buff中的读指针开始到读指针结束，这块区域是未读取到数据并去处"\r\n",返回有效数据的行末指针
        const char* lineEnd=search(buff.Peek(),buff.BeginWriteConst(),CRLF,CRLF+2);
        //转化为string类型
        std::string line(buff.Peek(),lineEnd);
        switch (state_)
        {
            //有限状态机，从请求行开始，每处理完成会自动转入到下一个状态
        case REQUEST_LINE:
            if(!ParseRequestLine_(line)){
                return false;
            }
            ParsePath_();       //解析路径
            break;
        case HEADERS:
            ParseHeader_(line);
            if(buff.ReadableBytes()<=2){
                state_=FINISH;
            }
        case BODY:
            ParseBody_(line);
        default:
            break;
        }
        if(lineEnd==buff.BeginWrite()){
            break;
        }
        buff.RetrieveUntil(lineEnd+2);              //跳过回车换行
    }
    LOG_DEBUG("[%s],[%s],[%s]",method_.c_str(),path_.c_str(),version_.c_str());
    return true;
}

//解析路径
void HttpRequest::ParsePath_(){
    if(path_=="/"){
        path_="/index.html";
    }else{
        for(auto &item:DEFAULT_HTML){
            if(item == path_){
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine_(const string& line){
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if(regex_match(line,subMatch,patten)){
        method_=subMatch[1];
        path_=subMatch[2];
        version_=subMatch[3];
        state_=HEADERS;         //状态转换为下一个状态
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader_(const string& line){
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if(regex_match(line,subMatch,patten)){
        header_[subMatch[1]]=subMatch[2];
    }else{
        state_=BODY;            //转换为下一个状态
    }
}

void HttpRequest::ParseBody_(const string& line){
    body_=line;
    ParsePost_();
    state_=FINISH;
    LOG_DEBUG("Body:%s,len:%d",line.c_str(),line.size());
}

//16进制转化为10进制
int HttpRequest::ConverHex(char ch){
    if(ch>='A' && ch <= 'F') return ch-'A' +10;
    if(ch>='a' && ch <= 'f' ) return ch-'a' +10;
    return ch;
}

//处理post请求
void HttpRequest::ParsePost_(){
    if(method_=="POST" && header_["Content-Type"] == "application/x-www-form-urlencoded"){
        ParseFromUrlencoded_();
        if(DEFAULT_HTML_TAG.count(path_)){
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d",tag);
            if(tag==0||tag==1){
                bool isLogin = (tag==1);
                if(UserVerify(post_["username"],post_["password"],isLogin)){
                    path_="/welcome.html";
                }else{
                    path_="/error.html";
                }
            }
        }
    }
}

//从url 中解析编码
void HttpRequest::ParseFromUrlencoded_(){
    if(body_.size()==0){return ;}

    string key,value;
    int num=0;
    int n=body_.size();
    int i=0,j=0;

    for(;i<n;i++){
        char ch=body_[i];
        switch (ch)
        {
        case '=':
            key = body_.substr(j,i-j);
            j=i+1;
            break;
        case '+':
            body_[i]=' ';
            break;
        case '%':
            num = ConverHex(body_[i+1]) * 16 +ConverHex(body_[i+2]);
            body_[i+2]=num%10 +'0';
            body_[i+1]=num/10 + '0';
            i+=2;
            break;
        case '&':
            value = body_.substr(j,i-j);
            j=i+1;
            post_[key] = value;
            LOG_DEBUG("%s = %s",key.c_str(),value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j<=i);
    if(post_.count(key) == 0&& j<i){
        value = body_.substr(j,i-j);
        post_[key] = value;
    }
}

bool HttpRequest::UserVerify(const string& name,const string& pwd,bool isLogin){
    if(name == ""|| pwd==""){
        return false;
    }
    LOG_INFO("Verify name:%s pwd:%s",name.c_str(),pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql,SqlConnPool::Instance());
    assert(sql);

    bool flag=false;
    unsigned int j=0;
    char order[256]={0};
    MYSQL_FIELD* fields=nullptr;
    MYSQL_RES* res=nullptr;

    if(!isLogin){
        flag=true;
    }
    snprintf(order,256,"SELECT username,password FROM user WHERE username='%s' LIMIT 1",name.c_str());
    LOG_DEBUG("%s",order);

    if(mysql_query(sql,order)){
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    j= mysql_num_fields(res);
    fields=mysql_fetch_fields(res);

    while(MYSQL_ROW row=mysql_fetch_row(res)){
        LOG_DEBUG("MYSQL ROW:%s %s",row[0],row[1]);
        string password(row[1]);
        //注册
        if(isLogin){
            if(pwd == password){
                flag=true;
            }else{
                flag = false;
                LOG_INFO("pwd error!");
            }
        }else{
            flag = false;
            LOG_INFO("user used!");
        }
    }
    mysql_free_result(res);

    //注册行为且用户名未被使用
    if(!isLogin && flag==true){
        LOG_DEBUG("regirster!");
        bzero(order,256);
        snprintf(order,256,"INSERT INTO user(username,password) VALUES('%s','%s')",name.c_str(),pwd.c_str());
        LOG_DEBUG("%s",order);
        if(mysql_query(sql,order)){
            LOG_DEBUG("Insert error!");
            flag=false;
        }
        flag = true;
    }
    LOG_DEBUG("UserVerify success!!");
    return flag;
}

string HttpRequest::path() const{
    return path_;
}

string& HttpRequest::path(){
    return path_;
}

string HttpRequest::method() const{
    return method_;
}

string HttpRequest::version() const{
    return version_;
}

string HttpRequest::GetPost(const string& key) const{
    assert(key!="");
    if(post_.count(key)==1){
        return post_.find(key)->second;
    }
    return "";
}

string HttpRequest::GetPost(const char* key) const{
    assert(key!=nullptr);
    if(post_.count(key)==1){
        return post_.find(key)->second;
    }
    return "";
}

