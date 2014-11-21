/*
 * CFdDispacher.h
 *
 *  Created on: 2014Äê8ÔÂ4ÈÕ
 *      Author: Jiansong
 */

#ifndef CFDDISPACHER_H_
#define CFDDISPACHER_H_

#include "module.h"
#include "mtpqueue_nolock.h"
#include "front_worker.h"
#include "log.h"
class CFrontWorker;

class RequestTask: public BaseTask {
private:
	int a_fd;
	int f_id;
	CModule *_module;
public:
	void SetFd(int fd) {
		this->a_fd = fd;
	}
	int GetFd() {
		return this->a_fd;
	}
	void SetModule(CModule *m) {
		this->_module = m;
	}
	CModule *GetModule() {
		return _module;
	}
	void SetId(int id) {
		this->f_id = id;
	}
	int GetId() {
		return f_id;
	}
};
class CloseFwsTask: public BaseTask {
private:
	CModule *_module;
public:
	void SetModule(CModule *m) {
		this->_module = m;
	}
	CModule *GetModule() {
		return _module;
	}
};

class SwitchCacheServerTask: public BaseTask {
private:
	CModule *_module;
    std::string name;
	uint32_t mode;
public:
	void SetModule(CModule *m) {
		this->_module = m;
	}
	CModule *GetModule() {
		return _module;
	}
	void SetName(std::string name)
	{
		this->name=name;
	}
	std::string GetName()
	{
		return this->name;
	}
	void SetMode(uint32_t mode)
	{
		this->mode=mode;
	}
	uint32_t GetMode()
	{
		return this->mode;
	}
};

class AddServerTask: public BaseTask {
private:
	CModule *_module;
	std::string serverName;
	std::string address;
	int virtualNode;
	std::string hotbak_addr;
	int mode;
public:
	void SetModule(CModule *m) {
		this->_module = m;
	}
	CModule *GetModule() {
		return _module;
	}
	void SetServerName(std::string name) {
		this->serverName = name;
	}
	std::string GetServerName() {
		return this->serverName;
	}
	void SetAddress(std::string addr) {
		this->address = addr;
	}
	std::string GetAddress() {
		return this->address;
	}
	void SetVirtualNode(int vn) {
		this->virtualNode = vn;
	}
	int GetVirtualNode() {
		return this->virtualNode;
	}
	void SetHotBackupAddress(std::string addr) {
		this->hotbak_addr = addr;
	}
	std::string GetHotBackupAddress() {
		return this->hotbak_addr;
	}
	void SetMode(int mode)
	{
		this->mode=mode;
	}
	int GetMode()
	{
		return this->mode;
	}
};

class RemoveServerTask: public BaseTask {
private:
	CModule *_module;
	std::string serverName;
	int virtualNode;
public:
	void SetModule(CModule *m) {
		this->_module = m;
	}
	CModule *GetModule() {
		return _module;
	}
	void SetServerName(std::string name) {
		this->serverName = name;
	}
	std::string GetServerName() {
		return this->serverName;
	}
	void SetVirtualNode(int vn) {
		this->virtualNode = vn;
	}
	int GetVirtualNode() {
		return this->virtualNode;
	}
};

class RelaseAllServerTask: public BaseTask {
private:
	CModule *_module;
public:
	void SetModule(CModule *m) {
		this->_module = m;
	}
	CModule *GetModule() {
		return _module;
	}
};

class CFdDispacher: public CThreadingPipeNoLockQueue<CFdDispacher> {
public:
	CFdDispacher();
	virtual ~CFdDispacher();
	void TaskNotify(BaseTask *task);
	void SetOwnerThread(CPollThread *thread);
private:
	CPollThread *ownerThread;
};

#endif /* CFDDISPACHER_H_ */
