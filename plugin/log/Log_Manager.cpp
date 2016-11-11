/*
 * Log_Manager.cpp
 *
 *  Created on: Nov 9, 2016
 *      Author: zhangyalei
 */

#include "Data_Manager.h"
#include "Node_Manager.h"
#include "Log_Manager.h"

Log_Manager::Log_Manager(void) { }

Log_Manager::~Log_Manager(void) { }

Log_Manager *Log_Manager::instance_;

Log_Manager *Log_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new Log_Manager;
	return instance_;
}

void Log_Manager::run_handler(void) {
	process_list();
}

int Log_Manager::process_list(void) {
	Msg_Head msg_head;
	Bit_Buffer bit_buffer;
	Byte_Buffer *buffer = nullptr;
	while (true) {
		//此处加锁是为了防止本线程其他地方同时等待条件变量成立
		notify_lock_.lock();

		//put wait in while cause there can be spurious wake up (due to signal/ENITR)
		while (buffer_list_.empty()) {
			notify_lock_.wait();
		}

		buffer = buffer_list_.pop_front();
		if(buffer != nullptr) {
			msg_head.reset();
			buffer->read_head(msg_head);
			bit_buffer.reset();
			bit_buffer.set_ary(buffer->get_read_ptr(), buffer->readable_bytes());

			switch(msg_head.msg_id) {
			case SYNC_LOG_PLAYER_LOGOUT:
				player_logout(bit_buffer);
				break;
			default:
				break;
			}

			//回收buffer
			NODE_MANAGER->push_buffer(msg_head.eid, msg_head.cid, buffer);
		}

		//操作完成解锁条件变量
		notify_lock_.unlock();
	}
	return 0;
}

void Log_Manager::player_logout(Bit_Buffer &buffer) {
	DATA_MANAGER->save_db_data(SAVE_DB_CLEAR_CACHE, DB_LOG, "log.logout", buffer);
}