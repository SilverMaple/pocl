/* OpenCL runtime library: clEnqueueFillBuffer()

   Copyright (c) 2015 Michal Babej / Tampere University of Technology

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include "pocl_util.h"
#include <string.h>

extern CL_API_ENTRY cl_int CL_API_CALL
POname(clEnqueueFillBuffer)(cl_command_queue  command_queue,
                           cl_mem            buffer,
                           const void *      pattern,
                           size_t            pattern_size,
                           size_t            offset,
                           size_t            size,
                           cl_uint           num_events_in_wait_list,
                           const cl_event*   event_wait_list,
                           cl_event*         event)
CL_API_SUFFIX__VERSION_1_2
{
  int errcode = CL_SUCCESS;
  _cl_command_node *cmd = NULL;

  POCL_RETURN_ERROR_COND((command_queue == NULL), CL_INVALID_COMMAND_QUEUE);

  POCL_RETURN_ERROR_COND((buffer == NULL), CL_INVALID_MEM_OBJECT);

  POCL_RETURN_ERROR_ON((buffer->type != CL_MEM_OBJECT_BUFFER), CL_INVALID_MEM_OBJECT,
                       "buffer is not a CL_MEM_OBJECT_BUFFER\n");

  POCL_RETURN_ERROR_ON((buffer->flags & CL_MEM_READ_ONLY), CL_INVALID_MEM_OBJECT,
                       "buffer is CL_MEM_READ_ONLY\n");

  POCL_RETURN_ERROR_ON((command_queue->context != buffer->context), CL_INVALID_CONTEXT,
                       "buffer and command_queue are not from the same context\n");

  POCL_RETURN_ERROR_COND((event_wait_list == NULL && num_events_in_wait_list > 0),
                         CL_INVALID_EVENT_WAIT_LIST);

  POCL_RETURN_ERROR_COND((event_wait_list != NULL && num_events_in_wait_list == 0),
                         CL_INVALID_EVENT_WAIT_LIST);

  errcode = pocl_buffer_boundcheck(buffer, offset, size);
  if (errcode != CL_SUCCESS)
    return errcode;

  /* CL_INVALID_VALUE if pattern is NULL or if pattern_size is 0
   * or if pattern_size is not one of {1, 2, 4, 8, 16, 32, 64, 128}. */
  POCL_RETURN_ERROR_COND((pattern == NULL), CL_INVALID_VALUE);
  POCL_RETURN_ERROR_COND((pattern_size == 0), CL_INVALID_VALUE);
  POCL_RETURN_ERROR_COND((pattern_size > 128), CL_INVALID_VALUE);

  POCL_RETURN_ERROR_ON((__builtin_popcount(pattern_size) > 1), CL_INVALID_VALUE,
                       "pattern_size(%zu) must be a power-of-two value", pattern_size);

  /* CL_INVALID_VALUE if offset and size are not a multiple of pattern_size.  */
  POCL_RETURN_ERROR_ON((offset % pattern_size), CL_INVALID_VALUE,
                       "offset(%zu) must be a multiple of pattern_size(%zu)\n",
                       offset, pattern_size);
  POCL_RETURN_ERROR_ON((size % pattern_size), CL_INVALID_VALUE,
                       "size(%zu) must be a multiple of pattern_size(%zu)\n",
                       size, pattern_size);

  /* ############# TODO #############
   * CL_MISALIGNED_SUB_BUFFER_OFFSET if buffer is a sub-buffer object
   * and offset specified when the sub-buffer object is created is not aligned
   * to CL_DEVICE_MEM_BASE_ADDR_ALIGN value for device associated with queue. */

  errcode = pocl_create_command (&cmd, command_queue, CL_COMMAND_FILL_BUFFER,
                                 event, num_events_in_wait_list,
                                 event_wait_list, 1, &buffer);
  if (errcode != CL_SUCCESS)
    return errcode;

  cmd->command.memfill.buffer = buffer;
  cmd->command.memfill.ptr =
      buffer->device_ptrs[command_queue->device->dev_id].mem_ptr;
  cmd->command.memfill.size = size;
  cmd->command.memfill.offset = offset;
  void *p = pocl_aligned_malloc(pattern_size, pattern_size);
  memcpy(p, pattern, pattern_size);
  cmd->command.memfill.pattern = p;
  cmd->command.memfill.pattern_size = pattern_size;

  POname(clRetainMemObject) (buffer);
  buffer->owning_device = command_queue->device;
  pocl_update_mem_obj_sync (command_queue, cmd, buffer, 'w');

  pocl_command_enqueue(command_queue, cmd);

  return CL_SUCCESS;

}
POsym(clEnqueueFillBuffer)
