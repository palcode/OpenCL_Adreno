#include <jni.h>
#include <android/log.h>

#include <sys/time.h>
#include <CL/cl.h>

#include <cstdlib> //malloc, free

// Commonly-defined shortcuts for LogCat output from native C applications.
#define  LOG_TAG    "Debug"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// trzymanie wszystkiego w strukturze wydaje się niegłupie
struct OpenCLObjects
{
    // Regular OpenCL objects:
    cl_platform_id *opencl_platforms;
    cl_device_id *platform_devices;
    cl_context cl_compute_context;
    cl_command_queue cl_compute_command_queue;
    cl_program cl_kernel_program;
    cl_kernel kernel;
    size_t globalWorksize;

    cl_mem device_in_a;
    cl_mem device_in_b;
    cl_mem device_out_c;
};

// Hold all OpenCL objects.
OpenCLObjects obj;

void initOpenCL
(
    JNIEnv* env,
    jobject thisObject,
    jfloatArray _A,
    jfloatArray _B,
    jint N,
    OpenCLObjects &obj
)
{
    const char* kernelSource = "__kernel void vectors(__global const float *a, __global const float *b, __global float *c, const int N){"
    "int id = get_global_id(0);"
    "if (id < N){"
    "c[id] = a[id] + b[id];"
    "c[id] *= a[id];"
    "c[id] *= b[id];"
    "c[id] *= c[id];"
    "c[id] *= 10.0;"
    "}"
    "}";

    size_t dataBytes = sizeof(float)*N;

    using namespace std;
    timeval start;
    timeval end;

    jfloat* A = env->GetFloatArrayElements(_A, 0);
    jfloat* B = env->GetFloatArrayElements(_B, 0);

    cl_uint ocl_plat_idx = 0;
    cl_uint ocl_dev_idx = 0;

    cl_uint clui_num_platforms = 0;
    cl_uint clui_num_devices = 0;

    cl_int cli_err_num;

    cl_device_type device_type;

    obj.opencl_platforms = NULL;
    obj.platform_devices = NULL;

    obj.cl_compute_command_queue = NULL;

    obj.kernel = NULL;

    cli_err_num = CL_SUCCESS;

    cli_err_num = clGetPlatformIDs(0, NULL, &clui_num_platforms);

    if (cli_err_num != CL_SUCCESS){
        LOGD("OpenCL nie działa !");
        return;
    }
    LOGD("Dostępnych platform: (%d).", clui_num_platforms);

    obj.opencl_platforms = (cl_platform_id*)malloc(clui_num_platforms*sizeof(cl_platform_id));

    if (obj.opencl_platforms == NULL){
        LOGD("Nie mogę alokować pamięci !");
        return;
    }

    cli_err_num = clGetPlatformIDs(clui_num_platforms, obj.opencl_platforms, NULL);

    if (cli_err_num != CL_SUCCESS){
        free((void*)obj.opencl_platforms);
        obj.opencl_platforms = NULL;
        LOGD("Inny error!");
        return;
    }

    cli_err_num = clGetDeviceIDs (obj.opencl_platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &clui_num_devices);

    if (cli_err_num != CL_SUCCESS){
        free((void*)obj.opencl_platforms);
        obj.opencl_platforms = NULL;
        LOGD("Błąd odczytu liczby urządzeń!");
        return;
    }
    LOGD("Liczba urządzeń: (%d).", clui_num_devices);

    obj.platform_devices = (cl_device_id*)malloc(clui_num_devices*sizeof(cl_device_id));

    cli_err_num = clGetDeviceIDs(obj.opencl_platforms[0], CL_DEVICE_TYPE_ALL, clui_num_devices, obj.platform_devices, NULL);

    if (cli_err_num != CL_SUCCESS){
        LOGD("Błąd odczytu informacji o urządzeniach!");
        return;
    }

    obj.cl_compute_context = clCreateContext(NULL, clui_num_devices, &obj.platform_devices[0], NULL, NULL, &cli_err_num);

    if(!obj.cl_compute_context){
        LOGD("Nie udało się utworzyć kontekstu!");
        return;
    }

    obj.cl_compute_command_queue = clCreateCommandQueue(obj.cl_compute_context, obj.platform_devices[0], 0, &cli_err_num);
    if(!obj.cl_compute_command_queue){
        LOGD("Nie udało się utworzyć kolejki!");
        return;
    }

    obj.device_in_a = clCreateBuffer(obj.cl_compute_context, CL_MEM_READ_ONLY, dataBytes, NULL, &cli_err_num);
    obj.device_in_b = clCreateBuffer(obj.cl_compute_context, CL_MEM_READ_ONLY, dataBytes, NULL, &cli_err_num);
    obj.device_out_c = clCreateBuffer(obj.cl_compute_context, CL_MEM_WRITE_ONLY, dataBytes, NULL, &cli_err_num);

    cli_err_num = clEnqueueWriteBuffer(obj.cl_compute_command_queue, obj.device_in_a, CL_TRUE, 0, dataBytes, A, 0, NULL, NULL);
    cli_err_num = clEnqueueWriteBuffer(obj.cl_compute_command_queue, obj.device_in_b, CL_TRUE, 0, dataBytes, B, 0, NULL, NULL);

    obj.cl_kernel_program = clCreateProgramWithSource(obj.cl_compute_context, 1, (const char**)&kernelSource, NULL, &cli_err_num);
    if(!obj.cl_kernel_program){
        LOGD("Nie udało się utworzyć programu!");
        return;
    }

    cli_err_num = clBuildProgram(obj.cl_kernel_program, clui_num_devices, obj.platform_devices, NULL, NULL, NULL);
    obj.kernel = clCreateKernel(obj.cl_kernel_program, "vectors", &cli_err_num);
    if(!obj.kernel || cli_err_num != CL_SUCCESS){
        LOGD("Nie udało się utworzyć jądra!");
        return;
    }

    cli_err_num = clSetKernelArg(obj.kernel, 0, sizeof(cl_mem), &obj.device_in_a);
    cli_err_num |= clSetKernelArg(obj.kernel, 1, sizeof(cl_mem), &obj.device_in_b);
    cli_err_num |= clSetKernelArg(obj.kernel, 2, sizeof(cl_mem), &obj.device_out_c);
    cli_err_num |= clSetKernelArg(obj.kernel, 3, sizeof(int), &N);

    obj.globalWorksize = N;
}

void vectorOpenCL(JNIEnv* env, jobject thisObject, jfloatArray _C, OpenCLObjects &obj){
    jfloat* C = env->GetFloatArrayElements(_C, 0);
    //gettimeofday(&start, NULL);
    cl_int cli_err_num;
    cli_err_num = clEnqueueNDRangeKernel(obj.cl_compute_command_queue, obj.kernel, 1, NULL, &obj.globalWorksize, NULL, 0, NULL, NULL);
    if(cli_err_num){
        LOGD("Nie udało się uruchomić jądra!");
        return;
    }
    //gettimeofday(&end, NULL);
    //float ndrangeDuration = (end.tv_sec + end.tv_usec * 1e-6) - (start.tv_sec + start.tv_usec * 1e-6);
    //LOGD("Czas wykonania: %f", ndrangeDuration);

    cli_err_num = clEnqueueReadBuffer(obj.cl_compute_command_queue, obj.device_out_c, CL_TRUE, 0, obj.globalWorksize*sizeof(float), C, 0, NULL, NULL);
    if(cli_err_num != CL_SUCCESS){
        LOGD("Nie udało się odczytać danych wyjściowych!");
        return;
    }
}

void cleanOpenCL( JNIEnv* env, jobject thisObject, OpenCLObjects &obj){
    clReleaseKernel(obj.kernel);
    clReleaseProgram(obj.cl_kernel_program);
    clReleaseCommandQueue(obj.cl_compute_command_queue);
    clReleaseMemObject(obj.device_in_a);
    clReleaseMemObject(obj.device_in_b);
    clReleaseMemObject(obj.device_out_c);
    clReleaseContext(obj.cl_compute_context);

    free((void*)obj.platform_devices);
    free((void*)obj.opencl_platforms);
}



extern "C" void Java_com_chabecki_smallbenchmark_MainActivity_initOpenCL
(
    JNIEnv* env,
    jobject thisObject,
    jfloatArray _A,
    jfloatArray _B,
    jint N
)
{
    initOpenCL
    (
        env,
        thisObject,
        _A,
        _B,
        N,
        obj
    );
}

extern "C" void Java_com_chabecki_smallbenchmark_MainActivity_vectorOpenCL
(
    JNIEnv* env,
    jobject thisObject,
    jfloatArray _C

)
{
    vectorOpenCL
    (
        env,
        thisObject,
        _C,
        obj
    );
}

extern "C" void Java_com_chabecki_smallbenchmark_MainActivity_cleanOpenCL
(
    JNIEnv* env,
    jobject thisObject

)
{
    cleanOpenCL
    (
        env,
        thisObject,
        obj
    );
}