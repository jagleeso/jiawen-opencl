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

    /* Based on timing measurements, it looks like the OpenCL compiler will optimize 
     * away any reads that can't subsequently affect a write.  To work around this, make 
     * every read appear to matter by adding each zero entry to local variable pmin, and 
     * writing it only for the first work-item.
     */
    uint4 pmin;

    uint n;
    for (n = 0; n < count; n++, idx += stride) {
        pmin += src[idx];
    }

    if (global_id == 0) {
        src[0] = pmin;
    }

}

/* Precondition:
 * elems is divisble by 4 * G * (spacing + 1).
 */
__kernel void coalesce_spacing(
        __global uint4 *src,
        uint elems,
        uint spacing)
{
    uint global_id = get_global_id(0);
    uint G = get_global_size(0);

    uint4 pmin;

    uint n, i;
    uint count = (elems/4) / (G*(spacing + 1));
    uint idx = global_id*(spacing + 1);
    uint stride = G*(spacing + 1);
    for (n = 0; n < count; n++, idx += stride) {
        for (i = 0; i < spacing + 1; i++) {
            pmin += src[idx + i];
        }
    }

    if (global_id == 0) {
        src[0] = pmin;
    }

}
