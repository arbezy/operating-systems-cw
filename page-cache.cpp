/* SPDX-License-Identifier: MIT */

/*
 * drivers/ata/page-cache.cpp
 * 
 * InfOS
 * Copyright (C) University of Edinburgh 2023.  All Rights Reserved.
 * 
 * ANDREW ELLISON <nonofurbusiness@inf.ed.ac.uk>
 */

#include <infos/drivers/ata/page-cache.h>
#include <infos/util/lock.h>
#include <infos/kernel/kernel.h>
#include <infos/util/string.h>


using namespace infos::kernel;
using namespace infos::drivers;
using namespace infos::drivers::ata;
using namespace infos::util;


PageCache::PageCache() {
    // //syslog.messagef(LogLevel::DEBUG, "LLOOK AT MEE");
    // //syslog.messagef(LogLevel::DEBUG, "%d", cache_buffer[0]);
    // cache_buffer[0] = -1;
    // //syslog.messagef(LogLevel::DEBUG, "%d", cache_buffer[0]);
    for (int i = 0; i < 64*512; i++) {
        //syslog.messagef(LogLevel::DEBUG, "%d", i);
        cache_buffer[i] = -1;
        
    }
    // //syslog.messagef(LogLevel::DEBUG, "done");
}

/** 
 * look for an empty space in the cache, if one exists then return the index of it
 * otherwies return -1
*/
int PageCache::seek_empty(){
    syslog.messagef(LogLevel::DEBUG, "looking for empty space in the cache");
    for (int i = 0; i < (64 * 512); i = i + 512) {
        if (cache_buffer[i] == -1) {
            return i;
        }
    }
    return -1;
}

int PageCache::LRU_contains(size_t lba) {
    for (int i=0; i<LRU.count(); i++) {
        if (LRU.at(i) == lba) {
            syslog.messagef(LogLevel::DEBUG, "lba already in lru list");
            return i;
        }
    }
    return -1;
}

/**
 * add a block to the cache, (either update existing elem, or add brand new one) using LRU if an existing block needs ot be removed
*/
bool PageCache::add_to_cache(size_t lba, uint8_t* buffer){

    //syslog.messagef(LogLevel::DEBUG, "adding to cache");
    
    int cache_offset = seek_empty();
    if (cache_offset != -1) {
        // empty space in cache. just do an easy add into this space
        //syslog.messagef(LogLevel::DEBUG, "empty space found @ %d", cache_offset);
        for (int i=0; i<512; i++) {
            cache_buffer[cache_offset + i] = buffer[i];
        }
        //syslog.messagef(LogLevel::DEBUG, "block added to cache");
        //cache_map.add(lba, cache_offset);

        int lru_offset = LRU_contains(lba);
        if (lru_offset != -1) {
            //LRU.remove(lru_offset);
        }
        LRU.enqueue(lba);
        syslog.messagef(LogLevel::DEBUG, "lba added to cache map");

        return true;
    }
    else {
        // no empty space. replace least recently used elem in the cache

        // 1. Get LRU block lba from the LRU queue
        uint64_t LRU_lba = LRU.dequeue();
        int64_t LRU_offset;
        cache_map.try_get_value(LRU_lba, LRU_offset);

        //syslog.messagef(LogLevel::DEBUG, "LRU element @ %d being overwritten :/", LRU_offset);

        // 2. Update map to record that block is not longer in cache
        // note: map.add(key, new_val) will update an exisitng map node with key from val to new_val
        cache_map.add(LRU_lba, -1);
        
        // 3. Overwrite LRU element in cache
        //cache_buffer[LRU_offset] = buffer + (lba * 512);
        for (uint64_t i=0; i<512; i++) {
            // note: might need to be buffer[lba + i]
            cache_buffer[LRU_offset + i] = buffer[i];
        }

        // 4. Record location of new block in the cache within the map
        cache_map.add(lba, LRU_offset);

        return true;
    }

    return false;
}

void PageCache::update_lru(size_t lba) {
    int lru_offset = LRU_contains(lba);
    if (lru_offset != -1) {
        LRU.remove(lru_offset);
    }
    LRU.enqueue(lba);
}

bool PageCache::read_from_cache_into_buffer(size_t lba, uint8_t* buffer){

    //syslog.messagef(LogLevel::DEBUG, "read from cache into buffer");

    int64_t cache_offset = -1;
    cache_map.try_get_value(lba, cache_offset);
    if (cache_offset != -1) {
        buffer[lba] = cache_buffer[cache_offset];
        return true;
    }
    else {
        // todo: add assert
        return false;
    }
}


/**
 * Look for block within cache, return int offset of block in cache if it is present, otherwise return -1 if there is no such block
*/
int64_t PageCache::seek_cache(size_t lba){
    //syslog.messagef(LogLevel::DEBUG, "seeking cache map for an lba");
    int64_t cache_offset;
    if (cache_map.try_get_value(lba, cache_offset)) {
        //syslog.messagef(LogLevel::DEBUG, "found @ cache offset == %d", cache_offset);
        return cache_offset;
    }
    //syslog.messagef(LogLevel::DEBUG, "lba not found in cache", cache_offset);
    return -1;
}


int PageCache::get_map_size(){
    return cache_map.count();
 }
