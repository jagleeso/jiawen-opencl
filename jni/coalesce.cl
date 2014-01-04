__kernel void coalesce_optimal(
        __global uint4 *src,
        uint elems)
{
    uint global_id = get_global_id(0);
    // 10. Set up __global memory access pattern.
    uint count = ( elems / 4 ) / get_global_size(0);
    /* CPU
     */
    // uint idx = get_global_id(0) * count;
    // uint stride = 1;
    /* GPU
     */
    uint idx = global_id;
    uint stride = get_global_size(0);

    /* uint pmin = (uint) -1; */
    uint4 pmin;

    uint n;
    for (n = 0; n < count && (idx + 1)*4 < elems; n++, idx += stride) {
        /* pmin = min( pmin, src[idx].x ); */
        /* pmin = min( pmin, src[idx].y ); */
        /* pmin = min( pmin, src[idx].z ); */
        /* pmin = min( pmin, src[idx].w ); */

        /* pmin = src[idx].x; */
        /* pmin = src[idx].y; */
        /* pmin = src[idx].z; */
        /* pmin = src[idx].w; */

        pmin = src[idx];
        /* src[idx] = (uint4)(0, 0, 0, 0); */
    }

}
