#ifndef _MODEL_H_
#define _MODEL_H_

#define NUM_PROTO_OPTS 4
struct model {
    u32 tx_power;
    u32 fixed_id;
    u16 ppmin_centerpw;
    u16 ppmin_deltapw;
    u8 num_ppmin;
    u8 num_channels;
    u8 protocol;
    u8 module;
    s16 proto_opts[NUM_PROTO_OPTS];
    struct limit limits[9];
};
extern struct model Model;
#endif /*_MODEL_H_*/
