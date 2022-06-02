#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_PLUGIN_CODE_WRITER_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_PLUGIN_CODE_WRITER_H_

#include <string>

namespace tinyRPC::protobuf::plugin {

// This interface defines interface for accepting generated code and write it to
// destinated files. If desired, class implementing this interface may create
// its own files.
class CodeWriter {
 public:
  virtual ~CodeWriter() = default;

  enum class File { Header, Source };

  // Returns a buffer for inserting code, the code will be inserted into header
  // file if `header` is set, or source file otherwise.
  //
  // We only uses insertion points defined as `kInsertionPointXxx`.
  virtual std::string* NewInsertionToHeader(
      const std::string& insertion_point) = 0;
  virtual std::string* NewInsertionToSource(
      const std::string& insertion_point) = 0;
};

}  // namespace tinyRPC::protobuf::plugin

#endif  
