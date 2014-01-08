__kernel void memcopy(__global uint4 *src, uint elems)
{
    uint count = ( elems / 4 ) / get_global_size(0);
    uint idx = get_global_id(0);
    uint stride = get_global_size(0);

    uint i;
    for (i = 0; i < count; i++, idx += stride) {
        src[idx] += 1;
    }

}

