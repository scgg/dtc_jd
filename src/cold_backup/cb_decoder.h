/*
 * =====================================================================================
 *
 *       Filename:  cb_decoder.h
 *
 *    Description:  cold backup decoder implementation
 *
 *        Version:  1.0
 *        Created:  05/13/2009 12:06:12 AM
 *       Revision:  $Id: cb_decoder.h 9310 2011-06-16 03:31:01Z yizhiliu $
 *       Compiler:  gcc
 *
 *         Author:  jackda (ada), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#ifndef __TTC_CB_DECODER_H
#define __TTC_CB_DECODER_H

#include <vector>
#include <buffer.h>
#include "cb_value.h"

namespace CB
{
	class cb_decoder
	{
		public:
			cb_decoder() {

			}

			virtual ~cb_decoder() {

			}

			cb_decoder & operator << (buffer &input) {
				/* when new data arrive, reset */
				reset();

				_dc_buffer.ptr = input.c_str();
				_dc_buffer.len = input.size();

				return *this;
			}

			int reset() {
				_dc_buffer.ptr 	= 0;
				_dc_buffer.len 	= 0;

				_dc_decoded_field.clear();
				return 0;
			}

			size_t size () {
				return _dc_decoded_field.size();
			}

			int decode() {
				if(!_dc_buffer) {
					_dc_decoded_field.clear();
					return 0;
				}

				char *iter = _dc_buffer.ptr;
				while(iter < _dc_buffer.ptr + _dc_buffer.len) {
					unsigned s = *(unsigned *)iter;
					iter += sizeof(s);
					CBinary field(s, iter);
					_dc_decoded_field.push_back(field);
					iter += s;
				}

				/* last check again */
				if(iter != _dc_buffer.ptr + _dc_buffer.len) {
					/* encounted error, reset decoded buffer*/
					_dc_decoded_field.clear();
					return -1;
				}

				return 0;
			}

			CBinary operator[] (int i)
			{
				if((size_t)i >= size())
					return CBinary();

				return _dc_decoded_field[i];
			}

		private:
			CBinary 		_dc_buffer;
			std::vector<CBinary>	_dc_decoded_field;
	};
};

#endif
