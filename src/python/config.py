MAX_SPACING = 12

# compilation of coalesce_spacing_group_* is expensive due to the large number of line 
# duplications generated (i=1..12 sum i*12*128).
#
# Disable this if you're not using the -u option and want to make runs go faster.
ENABLE_UNROLL_GROUP_MODE = True
WORK_GROUP_SIZE = 128
