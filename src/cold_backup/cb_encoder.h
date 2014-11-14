/*
 * =====================================================================================
 *
 *       Filename:  cb_encoder.h
 *
 *    Description:  cold backup encoder implementation
 *
 *        Version:  1.0
 *        Created:  05/12/2009 11:50:46 PM
 *       Revision:  $Id: cb_encoder.h 9310 2011-06-16 03:31:01Z yizhiliu $
 *       Compiler:  gcc
 *
 *         Author:  jackda (ada), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#ifndef __TTC_CB_ENCODER_H
#define __TTC_CB_ENCODER_H

#include <list>
#include <buffer.h>
#include "cb_value.h"

namespace CB
{
	class cb_encoder
	{
		public:
			cb_encoder() {

			}

			virtual ~cb_encoder() {

			}

			int cancle() {

				ec_field.clear();
				ec_buffer.clear();
				ec_encoded_buffer.clear();
				return 0;
			}

			/* push without copy */
			cb_encoder & operator <<(CBinary r) {

				if(!r) {
					return *this;
				}

				ec_field.push_back(r);
				return *this;
			}

			size_t size() {
				return  ec_encoded_buffer.len;
			}

			CBinary get_encoded_buffer() {
				return ec_encoded_buffer;
			}

			int encode() {
				if(ec_field.size() == 0)
					return -1;

				/* 4 bytes    | section-len */
				/*section-len | section-content */
				std::list<CBinary>::iterator it;
				for(it=ec_field.begin(); it != ec_field.end(); ++it) {
					/* section-len*/
					ec_buffer.append(it->len);
					/* section-content*/
					ec_buffer.append(it->ptr, it->len);
				}

				ec_encoded_buffer.len = ec_buffer.size();
				ec_encoded_buffer.ptr = ec_buffer.c_str();

				return 0;
			}
	private:
			std::list<CBinary>	ec_field;
			buffer          	ec_buffer;
			CBinary 		ec_encoded_buffer;
	};

	template <typename T>
	class buffered_cb_encoder: public T
	{
		public:
			buffered_cb_encoder() {

			}

			virtual ~buffered_cb_encoder() {

				delete_buffers();
			}

			/* push with copy original buffer */
			int cancle() {

				delete_buffers();
				alloc_list.clear();
				return T::cancle();
			}

			buffered_cb_encoder & operator << (CBinary r) {

				if(!r) {
					return *this;
				}

				CBinary alloc = alloc_and_copy_buffer(r);
				if(!alloc) {
					return *this;
				}

				T::operator << (alloc);
				return *this;
			}

		private:
			void delete_buffers(void) {

				std::list<CBinary>::iterator it;

				for(it=alloc_list.begin(); it != alloc_list.end(); ++it) {
					FREE_IF(it->ptr); it->len = 0;
				}
			}

			CBinary alloc_and_copy_buffer(CBinary r) {

				char *str = (char *) MALLOC(r.len);
				if(!str) {
					return CBinary();
				}

				memcpy(str, r.ptr, r.len);
				CBinary alloc(r.len, str);
				alloc_list.push_back(alloc);

				return alloc;
			}
		private:
			std::list<CBinary> alloc_list;
	};
};

#endif
