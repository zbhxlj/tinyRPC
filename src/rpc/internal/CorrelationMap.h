#ifndef _SRC_RPC_INTERNAL_CORRELATION_MAP_H_
#define _SRC_RPC_INTERNAL_CORRELATION_MAP_H_

#include <vector>

#include "../../base/IDAlloc.h"
#include "../../fiber/Runtime.h"
#include "ShardedCallMap.h"

// Here we use a semi-global correlation map for all outgoing RPCs.
//
// This helps us to separate timeout logic from normal RPCs. Now we don't need
// correpsonding connection object to be alive to deal with timeout timers.
//
// This is not the case previously. As the timeout timer may access the
// correlation map in corresponding connection object, we must either keep the
// connection alive, or wait for timer to be quiescent before destroying the
// connection. Either way, we'll introduce a synchronization overhead.
//
// By moving timeout out from connection handling, it also brings us another
// benefit: Now we can handle timeout and backup-request in a similar fashion.
// Were timeout detected in connection objects, implementing backup-request
// would require both connection and channel objects to keep a timer, one for
// detecting timeout, and another for firing backup-request. That can be messy.

namespace tinyRPC::rpc::internal {

// TODO(luobogao): Now that we're using a (semi-)global map, we can use a
// fixed-sized lockless map instead of `ShardedCallMap` for more stable
// performance.
//
// The reason why we can't do this previously is that we can't afford to keep a
// large fixed-size map for each connection. Now that it's semi-global, it's
// affordable.
//
// TODO(luobogao): Expose statistics of each correlation map via `ExposedVar`.
template <class T>
using CorrelationMap = ShardedCallMap<T>;

// Get correlation map for the given scheduling group. The resulting map is
// indexed by key generated via `MergeCorrelationId`.
template <class T>
CorrelationMap<T>* GetCorrelationMapFor(std::size_t scheduling_group_id) {
  static std::vector<CorrelationMap<T>> maps(
      fiber::GetSchedulingGroupCount());
  FLARE_CHECK_LT(scheduling_group_id, maps->size());
  return &(*maps)[scheduling_group_id];
}

}  // namespace tinyRPC::rpc::internal

#endif  
