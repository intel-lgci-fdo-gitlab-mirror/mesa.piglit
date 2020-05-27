/*!

# FIXME: This requires OpenCL 2.0 due to a bug in the
# __builtin_alloca handling, which is incorrectly returning a
# generic pointer instead of private.

# TODO: Also stress __builtin_alloca_with_align, and in non-kernel
# functions.

[config]
name: kernel with builtin_alloca
build_options: -cl-std=CL2.0
clc_version_min: 20

[test]
name: builtin alloca outside control flow
kernel_name: non_entry_block_alloca_kernel_constant_size
dimensions: 1
global_size: 32 0 0
local_size: 32 0 0

arg_out: 0 buffer int[32] \
  0x07080900  0x07080901  0x07080902  0x07080903  0x07080904 \
  0x07080905  0x07080906  0x07080907  0x07080908  0x07080909 \
  0x0708090a  0x0708090b  0x0708090c  0x0708090d  0x0708090e \
  0x0708090f  0x07080910  0x07080911  0x07080912  0x07080913 \
  0x07080914  0x07080915  0x07080916  0x07080917  0x07080918 \
  0x07080919  0x0708091a  0x0708091b  0x0708091c  0x0708091d \
  0x0708091e  0x0708091f

arg_out: 1 buffer int[12] \
  0xdeadbeef  0xfeedbeef  0x1234abcd  0x8910ddbb \
  0x11111111  0x22222222  0x33333333  0x44444444 \
  0x55555555  0x66666666  0x77777777  0x88888888

arg_in: 2 int 1

!*/

// The point is to stress __builtin_alloca, but we don't have a way to
// skip the test if it's not supported.

#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif

kernel void non_entry_block_alloca_kernel_constant_size(
  global int* out0, global int* out1, int c) {
#if !__has_builtin(__builtin_alloca)
  volatile private int alloc_ptr[32];
#endif

  const int num_alloca_elts = 32;

  // Add a static sized object to make sure it isn't clobbered in the
  // dynamic region. Ideally this will exceed the wave size in bytes.
  volatile int other_array[12] = {
    0xffffffff, 0xffffeeee, 0x11111111, 0xf0f0f0f0,
    0x99999999, 0xaaaaaaaa, 0xbbbbbbbb, 0x0f0f0f0f,
    0x88988989, 0x59419293, 0x02349249, 0x92392399
  };

  int lid = get_local_id(0);

  if (c) {
#if __has_builtin(__builtin_alloca)
    // Allocate a fixed size amount outside of the entry block.
    // FIXME: to_private should be unnecessary.
    volatile private int* alloc_ptr =
      to_private(__builtin_alloca(num_alloca_elts * sizeof(int)));
#endif

    // Write some sample values to the dynamic stack object.
    for (int i = 0; i < num_alloca_elts; ++i) {
      const int test_val = 0x07080900 | i;
      alloc_ptr[i] = test_val;
    }

    // Write some new values to the static object to make sure nothing
    // is clobbered.
    other_array[0] = 0xdeadbeef;
    other_array[1] = 0xfeedbeef;
    other_array[2] = 0x1234abcd;
    other_array[3] = 0x8910ddbb;
    other_array[4] = 0x11111111;
    other_array[5] = 0x22222222;
    other_array[6] = 0x33333333;
    other_array[7] = 0x44444444;
    other_array[8] = 0x55555555;
    other_array[9] = 0x66666666;
    other_array[10] = 0x77777777;
    other_array[11] = 0x88888888;

    // Make sure we got the right values in the alloca block.
    out0[lid] = alloc_ptr[lid];
  }

  if (lid == 0) {
    // Make sure the other array wasn't clobbered.
    for (int i = 0; i < 12; ++i) {
     out1[i] = other_array[i];
    }
  }
}
