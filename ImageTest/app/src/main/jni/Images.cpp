#include <jni.h>
#include <android/bitmap.h>
#include <android/log.h>

#include <cstdlib>
#include <vector>

#include <CL/cl.h>

#define  LOG_TAG    "ImagesLibrary"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

const char* opencl_error_to_str (cl_int error)
{
#define CASE_CL_CONSTANT(NAME) case NAME: return #NAME;

    // Suppose that no combinations are possible.
    switch(error)
    {
        CASE_CL_CONSTANT(CL_SUCCESS)
        CASE_CL_CONSTANT(CL_DEVICE_NOT_FOUND)
        CASE_CL_CONSTANT(CL_DEVICE_NOT_AVAILABLE)
        CASE_CL_CONSTANT(CL_COMPILER_NOT_AVAILABLE)
        CASE_CL_CONSTANT(CL_MEM_OBJECT_ALLOCATION_FAILURE)
        CASE_CL_CONSTANT(CL_OUT_OF_RESOURCES)
        CASE_CL_CONSTANT(CL_OUT_OF_HOST_MEMORY)
        CASE_CL_CONSTANT(CL_PROFILING_INFO_NOT_AVAILABLE)
        CASE_CL_CONSTANT(CL_MEM_COPY_OVERLAP)
        CASE_CL_CONSTANT(CL_IMAGE_FORMAT_MISMATCH)
        CASE_CL_CONSTANT(CL_IMAGE_FORMAT_NOT_SUPPORTED)
        CASE_CL_CONSTANT(CL_BUILD_PROGRAM_FAILURE)
        CASE_CL_CONSTANT(CL_MAP_FAILURE)
        CASE_CL_CONSTANT(CL_MISALIGNED_SUB_BUFFER_OFFSET)
        CASE_CL_CONSTANT(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST)
        CASE_CL_CONSTANT(CL_INVALID_VALUE)
        CASE_CL_CONSTANT(CL_INVALID_DEVICE_TYPE)
        CASE_CL_CONSTANT(CL_INVALID_PLATFORM)
        CASE_CL_CONSTANT(CL_INVALID_DEVICE)
        CASE_CL_CONSTANT(CL_INVALID_CONTEXT)
        CASE_CL_CONSTANT(CL_INVALID_QUEUE_PROPERTIES)
        CASE_CL_CONSTANT(CL_INVALID_COMMAND_QUEUE)
        CASE_CL_CONSTANT(CL_INVALID_HOST_PTR)
        CASE_CL_CONSTANT(CL_INVALID_MEM_OBJECT)
        CASE_CL_CONSTANT(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)
        CASE_CL_CONSTANT(CL_INVALID_IMAGE_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_SAMPLER)
        CASE_CL_CONSTANT(CL_INVALID_BINARY)
        CASE_CL_CONSTANT(CL_INVALID_BUILD_OPTIONS)
        CASE_CL_CONSTANT(CL_INVALID_PROGRAM)
        CASE_CL_CONSTANT(CL_INVALID_PROGRAM_EXECUTABLE)
        CASE_CL_CONSTANT(CL_INVALID_KERNEL_NAME)
        CASE_CL_CONSTANT(CL_INVALID_KERNEL_DEFINITION)
        CASE_CL_CONSTANT(CL_INVALID_KERNEL)
        CASE_CL_CONSTANT(CL_INVALID_ARG_INDEX)
        CASE_CL_CONSTANT(CL_INVALID_ARG_VALUE)
        CASE_CL_CONSTANT(CL_INVALID_ARG_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_KERNEL_ARGS)
        CASE_CL_CONSTANT(CL_INVALID_WORK_DIMENSION)
        CASE_CL_CONSTANT(CL_INVALID_WORK_GROUP_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_WORK_ITEM_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_GLOBAL_OFFSET)
        CASE_CL_CONSTANT(CL_INVALID_EVENT_WAIT_LIST)
        CASE_CL_CONSTANT(CL_INVALID_EVENT)
        CASE_CL_CONSTANT(CL_INVALID_OPERATION)
        CASE_CL_CONSTANT(CL_INVALID_GL_OBJECT)
        CASE_CL_CONSTANT(CL_INVALID_BUFFER_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_MIP_LEVEL)
        CASE_CL_CONSTANT(CL_INVALID_GLOBAL_WORK_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_PROPERTY)

    default:
        return "UNKNOWN ERROR CODE";
    }

#undef CASE_CL_CONSTANT
}

#define SAMPLE_CHECK_ERRORS(ERR)                                                      \
    if(ERR != CL_SUCCESS)                                                             \
    {                                                                                 \
        LOGE                                                                          \
        (                                                                             \
            "OpenCL error with code %s happened in file %s at line %d. Exiting.\n",   \
            opencl_error_to_str(ERR), __FILE__, __LINE__                              \
        );                                                                            \
                                                                                      \
        return;                                                                       \
    }


void runOpenCL
(
    JNIEnv* env,
    jobject thisObject,
    jobject inputBitmap,
    jobject outputBitmap
)
{
    using namespace std;

    const char* kernelSource = "__kernel void processIMG ( __global const int* inputPixels, __global int* outputPixels, const uint rowPitch){"
                               "int x = get_global_id(0);"
                               "int y = get_global_id(1);"
                               "int inPixel = inputPixels[x + y*rowPitch];"
                               "int A = ((inPixel >> 24) & 0xFF);"
                               "float tmpR = (float)((inPixel >> 16) & 0xFF);"
                               "float tmpG = (float)((inPixel >> 8) & 0xFF);"
                               "float tmpB = (float)(inPixel & 0xFF);"
                               "float gray = (tmpR + tmpG + tmpB)/3.0f;"
                               "tmpR = gray;"
                               "tmpG = gray;"
                               "tmpB = gray;"
                               "int R = floor((tmpR*0.393f) + (tmpG*0.769f) + (tmpB*0.189f) + 0.5f);"
                               "int G = floor((tmpR*0.349f) + (tmpG*0.686f) + (tmpB*0.168f) + 0.5f);"
                               "int B = floor((tmpR*0.272f) + (tmpG*0.534f) + (tmpB*0.131f) + 0.5f);"
                               "(R > 255) ? (R = 255):(R = R);"
                               "(G > 255) ? (G = 255):(G = G);"
                               "(B > 255) ? (B = 255):(B = B);"
                               "outputPixels[x + y*rowPitch] = (R | (G<<8) | (B<<16) | (A<<24));"
                               "};";

    cl_platform_id* opencl_platforms;
    opencl_platforms = NULL;
    cl_device_id* platform_devices;
    platform_devices = NULL;
    cl_device_type device_type;
    cl_context cl_compute_context;
    cl_command_queue cl_compute_command_queue = NULL;
    cl_program cl_kernel_program;
    cl_kernel kernel = NULL;

    cl_mem inputBuffer;
    cl_mem outputBuffer;

    cl_uint ocl_plat_idx = 0;
    cl_uint ocl_dev_idx = 0;

    cl_uint clui_num_platforms = 0;
    cl_uint clui_num_devices = 0;

    cl_int cli_err_num;

    cli_err_num = CL_SUCCESS;

    cli_err_num = clGetPlatformIDs(0, NULL, &clui_num_platforms);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    opencl_platforms = (cl_platform_id*)malloc(clui_num_platforms*sizeof(cl_platform_id));

    cli_err_num = clGetPlatformIDs(clui_num_platforms, opencl_platforms, NULL);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    cli_err_num = clGetDeviceIDs (opencl_platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &clui_num_devices);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    platform_devices = (cl_device_id*)malloc(clui_num_devices*sizeof(cl_device_id));

    cli_err_num = clGetDeviceIDs(opencl_platforms[0], CL_DEVICE_TYPE_ALL, clui_num_devices, platform_devices, NULL);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    cl_compute_context = clCreateContext(NULL, clui_num_devices, &platform_devices[0], NULL, NULL, &cli_err_num);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    cl_compute_command_queue = clCreateCommandQueue(cl_compute_context, platform_devices[0], 0, &cli_err_num);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    AndroidBitmapInfo bitmapInfo;

    AndroidBitmap_getInfo(env, inputBitmap, &bitmapInfo);
    size_t bufferSize = bitmapInfo.height * bitmapInfo.stride;
    cl_uint rowPitch = bitmapInfo.stride / 4;

    void* inputPixels = 0;
    AndroidBitmap_lockPixels(env, inputBitmap, &inputPixels);
    inputBuffer = clCreateBuffer(cl_compute_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bufferSize, inputPixels, &cli_err_num);
    SAMPLE_CHECK_ERRORS(cli_err_num);
    AndroidBitmap_unlockPixels(env, inputBitmap);

    void* outputPixels = 0;
    AndroidBitmap_lockPixels(env, outputBitmap, &outputPixels);
    outputBuffer = clCreateBuffer(cl_compute_context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, bufferSize, outputPixels, &cli_err_num);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    size_t globalWorksize[2] = { bitmapInfo.width, bitmapInfo.height };
    cl_kernel_program = clCreateProgramWithSource(cl_compute_context, 1, (const char**)&kernelSource, NULL, &cli_err_num);

    SAMPLE_CHECK_ERRORS(cli_err_num);

    cli_err_num = clBuildProgram(cl_kernel_program, clui_num_devices, platform_devices, NULL, NULL, NULL);
    if(cli_err_num == CL_BUILD_PROGRAM_FAILURE)
        {
            size_t log_length = 0;
            cli_err_num = clGetProgramBuildInfo(
                cl_kernel_program,
                platform_devices[0],
                CL_PROGRAM_BUILD_LOG,
                0,
                0,
                &log_length
            );
            SAMPLE_CHECK_ERRORS(cli_err_num);

            vector<char> log(log_length);

            cli_err_num = clGetProgramBuildInfo(
                cl_kernel_program,
                platform_devices[0],
                CL_PROGRAM_BUILD_LOG,
                log_length,
                &log[0],
                0
            );
            SAMPLE_CHECK_ERRORS(cli_err_num);

            LOGE
            (
                "Error happened during the build of OpenCL program.\nBuild log:%s",
                &log[0]
            );

            return;
        }
    SAMPLE_CHECK_ERRORS(cli_err_num);
    kernel = clCreateKernel(cl_kernel_program, "processIMG", &cli_err_num);
        if(!kernel || cli_err_num != CL_SUCCESS){
            LOGD("Nie udało się utworzyć jądra!");
            return;
        }

    SAMPLE_CHECK_ERRORS(cli_err_num);

    cli_err_num = clSetKernelArg(kernel, 0, sizeof(inputBuffer), &inputBuffer);
    SAMPLE_CHECK_ERRORS(cli_err_num);
    cli_err_num |= clSetKernelArg(kernel, 1, sizeof(outputBuffer), &outputBuffer);
    SAMPLE_CHECK_ERRORS(cli_err_num);
    cli_err_num |= clSetKernelArg(kernel, 2, sizeof(cl_uint), &rowPitch);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    cli_err_num = clEnqueueNDRangeKernel(cl_compute_command_queue, kernel, 2, NULL, globalWorksize, NULL, 0, NULL, NULL);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    cli_err_num = clFinish(cl_compute_command_queue);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    cli_err_num = clEnqueueReadBuffer(cl_compute_command_queue, outputBuffer, CL_TRUE, 0, bufferSize, outputPixels, 0, NULL, NULL);
    SAMPLE_CHECK_ERRORS(cli_err_num);
    cli_err_num = clFinish(cl_compute_command_queue);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    cli_err_num = clReleaseMemObject(outputBuffer);
    SAMPLE_CHECK_ERRORS(cli_err_num);
    AndroidBitmap_unlockPixels(env, outputBitmap);

    cli_err_num = clReleaseMemObject(inputBuffer);
    SAMPLE_CHECK_ERRORS(cli_err_num);

    cli_err_num = clReleaseKernel(kernel);
    cli_err_num = clReleaseProgram(cl_kernel_program);
    cli_err_num = clReleaseCommandQueue(cl_compute_command_queue);

    cli_err_num = clReleaseContext(cl_compute_context);

    free((void*)platform_devices);
    free((void*)opencl_platforms);
}


extern "C" void Java_com_chabecki_imagetest_MainActivity_runOpenCL
(
    JNIEnv* env,
    jobject thisObject,
    jobject inputBitmap,
    jobject outputBitmap
)
{
    runOpenCL
    (
        env,
        thisObject,
        inputBitmap,
        outputBitmap
    );
}

