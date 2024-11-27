#include <cstring> // std::memcpy

#include "log_buffer.h"

namespace xubinh_server {

// explicit instantiation
template class FixedSizeLogBuffer<LogBufferSizeConfig::LOG_ENTRY_BUFFER_SIZE>;
template class FixedSizeLogBuffer<LogBufferSizeConfig::LOG_CHUNK_BUFFER_SIZE>;

} // namespace xubinh_server