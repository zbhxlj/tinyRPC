#ifndef _SRC_IO_ACCEPTOR_H_
#define _SRC_IO_ACCEPTOR_H_

namespace tinyRPC {

// `Acceptor` listens for incoming connections.
class Acceptor {
 public:
  virtual ~Acceptor() = default;

  // Kill the acceptor and wait for its full stop.
  virtual void Stop() = 0;
  virtual void Join() = 0;
};

}  // namespace tinyRPC

#endif 
