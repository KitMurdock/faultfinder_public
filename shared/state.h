#ifndef _STATE_H_
#define _STATE_H_


    struct uc_context_copy {
        size_t context_size; // size of the real internal context structure
        uc_mode mode;        // the mode of this context
        uc_arch arch;        // the arch of this context
        char data[0];        // context
    }; 
    void make_hash(uc_engine*uc, uint32_t* hashes);

#endif