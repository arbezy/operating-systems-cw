#pragma once


#include <infos/drivers/block/block-device.h>
#include <infos/drivers/block/block-device-partition.h>
#include <infos/util/list.h>
#include <infos/util/map.h>

namespace infos {
    namespace drivers {
        namespace ata {
            class PageCache {
            public:
                PageCache();
                //void init();
                int seek_empty();
                int LRU_contains(size_t lba);
                bool add_to_cache(size_t lba, uint8_t* buffer);
                void update_lru(size_t lba);
                bool read_from_cache_into_buffer(size_t lba, uint8_t* buffer);
                int64_t seek_cache(size_t lba);
                int get_map_size();

            private:
                int8_t cache_buffer[64 * 512];
                // legacy:
                // hashmap of pointer to a block, and offset (location of block in the cache) --> is actually a treemap not a hash :)
                // if block no longer in the cache offset == -1?
                // current:
                // map stores key=lba, val=cache buffer offset/index
                infos::util::Map<size_t, int64_t> cache_map;
                // The index of the Least recently used block in the cache, starts out at zero
                // when the LRU element is overwritten this should be incremented by 1 and moduloed by cache_size(64)
                infos::util::List<size_t> LRU;
            }__aligned(16);
        }
    }
}
