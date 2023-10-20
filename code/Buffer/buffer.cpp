#include "buffer.h"

using namespace std;

//构造函数初始化
Buffer::Buffer(int initBuffSize): buffer_(initBuffSize),readPos_(0),writePos_(0){}

//可写长度：buffer大小 - 写位置的下标
size_t Buffer::WritableBytes() const {
	return buffer_.size() - writePos_;
}

//可读长度：写位置下标 - 读位置下标
size_t Buffer::ReadableBytes() const{
	return writePos_ - readPos_;
}

//已读完毕长度：读完后空间可回收，读位置下标
size_t Buffer::PrependableBytes() const{
	return readPos_;
}

//缓冲区读位置指针
const char* Buffer::Peek() const{
	return &buffer_[readPos_];
}

//确保可写长度
void Buffer::EnsureWriteable(size_t len){
	if(len > WritableBytes()){
		MakeSpace_(len);
	}
	assert(len <= WritableBytes());
}

//移动写下标,用于append()中
void Buffer::HasWritten(size_t len){
	writePos_ += len;
}

//移动读下标，移动len长度
void Buffer::Retrieve(size_t len){
	readPos_ += len;
}

//读取到end位置
void Buffer::RetrieveUntil(const char* end){
	assert(Peek() <= end);
	Retrieve(end - Peek()); //end指针位置 - peek指针位置（当前读位置）= 所需读取长度
}

//清空所有数据，buffer_清空，读写下标归零
void Buffer::RetrieveAll(){
	bzero(&buffer_[0],buffer_.size());
	readPos_ = writePos_ =0;
}

//取出剩余可读的str
string Buffer::RetrieveAllToStr(){
	string str(Peek(),ReadableBytes()); //string str(iterator::begin(),iterator::end())
	RetrieveAll();
	return str;
}

//写指针位置
const char* Buffer::BeginWriteConst() const {
	return &buffer_[writePos_];
}

char* Buffer::BeginWrite(){
	return &buffer_[writePos_];
}

//添加str到缓冲区
//参数： str指针，str长度，
//在写下标位置插入str，写下标移动到末尾
void Buffer::Append(const char* str,size_t len){
	assert(str);
	EnsureWriteable(len);			//确保可写
	copy(str,str+len,BeginWrite());		//copy(begin(),length(),begin())
	HasWritten(len);			//移动写下标
}

void Buffer::Append(const string& str){
	Append(str.c_str(),str.size());
}

void Buffer::Append(const void* data,size_t len){
	Append(static_cast<const char*>(data),len);
}

//将buffer_中读下标与写下标中间的数据放到写下标后面
void Buffer::Append(const Buffer& buff){
	Append(buff.Peek(),ReadableBytes());
}

//将文件fd的内容读取到buffer_缓冲区，从writable位置追加
ssize_t Buffer::ReadFd(int fd,int* Errno){
	char buff[65535];
	iovec iov[2];

	size_t writable = WritableBytes();
	//分两个读取文件，防止数据溢出没读完
	iov[0].iov_base = BeginWrite();
	iov[0].iov_len = writable;
	iov[1].iov_base = buff;
	iov[1].iov_len = sizeof(buff);
	
	ssize_t len = readv(fd,iov,2);
	if(len < 0){
		*Errno = errno;
	} else if(static_cast<size_t>(len) <= writable){
		writePos_ += len;
	} else{
		writePos_ = buffer_.size();
		Append(buff,static_cast<size_t>(len - writable));
	}
	return len;
}

//将buffer_中可读的区域写入fd中
ssize_t Buffer::WriteFd(int fd,int* Errno){
	ssize_t len = write(fd,Peek(),ReadableBytes());
	if(len < 0){
		*Errno = errno;
		return len;
	}
	Retrieve(len);
	return len;
}

char* Buffer::BeginPtr_(){
	return &buffer_[0];
}

const char* Buffer::BeginPtr_() const{
	return &buffer_[0];
}

void Buffer::MakeSpace_(size_t len){
	if((WritableBytes() + PrependableBytes()) < len){
		buffer_.resize(writePos_+len+1);
	} else {
		size_t readable = ReadableBytes();
		copy(BeginPtr_() + readPos_,BeginPtr_()+writePos_,BeginPtr_());
		readPos_ = 0 ;
		writePos_ = readable;
		assert(readable == ReadableBytes());
	}
}

















