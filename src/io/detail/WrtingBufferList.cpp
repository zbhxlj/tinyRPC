#include <mutex>

#include "WritingBufferList.h"

namespace tinyRPC::io::detail{

WritingBufferList::WritingBufferList() {}

WritingBufferList::~WritingBufferList() {
    for(auto&& node : buffers_){
        delete node;
    }
}

// Tremendous performance hurt.
ssize_t WritingBufferList::FlushTo(tinyRPC::AbstractStreamIo* io, std::size_t max_bytes,
                std::vector<std::uintptr_t>* flushed_ctxs, bool* emptied,
                bool* short_write){
    std::unique_lock lk(lock_);
    std::string gatheredBuffer;
    std::size_t flushing = 0;
    
    auto iter = buffers_.begin();

    while(flushing < max_bytes && iter != buffers_.end()){
        auto&& s = (*iter)->buffer;

        std::size_t writeSize = s.size();
        if(writeSize + flushing > max_bytes){
            // exceeded size = s.size() + flushing - max_bytes
            // pos = s.size() - exceed size = max_bytes - flusing
            writeSize -= s.size() + flushing - max_bytes;
            auto writeBuffer = s.substr(0, writeSize);
            gatheredBuffer += writeBuffer;            
        }else{
            gatheredBuffer += s;
        }
        flushing += writeSize;
        iter++;
    }


    ssize_t rc = io->Write(gatheredBuffer);
    if (rc <= 0) {
        return rc;  // Nothing is really flushed then.
    }
    CHECK_LE(rc, flushing);


    // Drain written out bytes.
    std::size_t cnt = 0;
    iter = buffers_.begin();

    while(cnt < rc && iter != buffers_.end()){
        auto&& s = (*iter)->buffer;

        std::size_t writeSize = s.size();
        if(writeSize + cnt > rc){
            writeSize -= rc - cnt;
            (*iter)->buffer = s.substr(rc - cnt);
            iter++;
        }else{
            auto iter2 = iter++;
            flushed_ctxs->push_back((*iter2)->ctx);
            buffers_.erase(iter2);
        }
        cnt += writeSize;
    }

    *emptied = buffers_.empty();
    *short_write = rc != flushing;
    return rc;
}


bool WritingBufferList::Append(std::string buffer, std::uintptr_t ctx){
    std::unique_lock _(lock_);
    buffers_.emplace_back(new WritingBufferList::Node(buffer, ctx));
    return buffers_.size() == 1;
}
} // namespace tinyRPC::io::detail