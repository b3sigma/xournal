#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

bool initialize(char* weights_filename, void** out_scribble_handle);
void cleanup(void* scribble_handle);

bool memory_recognize(void* scribble_handle, unsigned char* image_data,
        int x_size, int y_size, int pitch_bytes, int y_stride_bytes,
        int* out_what_number);


#ifdef __cplusplus
}
#endif //__cplusplus
