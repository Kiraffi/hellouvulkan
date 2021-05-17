
layout (location = 0) uniform float2 txsize;
layout (location = 1) uniform float roll;

layout (rgba32f, binding = 0) uniform image2D storeTex0;
layout (std430, binding = 3) buffer myBuffer { float4 data_SSBO[]; };
layout (local_size_x = 8, local_size_y = 8) in;

#define TRIANGLES 256
#define THREAD_COUNT 64
//

#define USE_LDS 1

#if USE_LDS
    shared float3 shared_data[THREAD_COUNT * 3];
#endif
// gl_WorkGroupID.xy
// gl_WorkGroupSize.xy


void main()
{
    float2 storePos = float2(gl_GlobalInvocationID.xy);
    if(storePos.x < txsize.x && storePos.y < txsize.y)
    {
        uint2 groupThreadId = gl_LocalInvocationID.xy;
        uint groupThreadIndex = uint(groupThreadId.y * 16 + groupThreadId.x);
        float2 texCoord = storePos / txsize;
        float2 vertexPos = texCoord * 2.0f - 1.0f;
        float maxv = 1.0f;
        uint ds = 3 + 1; // 3 * 3 + 2 * 2;

        for(uint i = 0; i < TRIANGLES; i+= THREAD_COUNT)
        {
            #if USE_LDS

                uint dataSizePoint = (i + groupThreadIndex) * ds;

                shared_data[groupThreadIndex * 3 + 0] =
                    float3(data_SSBO[dataSizePoint + 0].xyz);//, data_SSBO[dataSizePoint + 1], data_SSBO[dataSizePoint + 2]);
                shared_data[groupThreadIndex * 3 + 1] =
                    float3(data_SSBO[dataSizePoint + 1].xyz); //, data_SSBO[dataSizePoint + 4], data_SSBO[dataSizePoint + 5]);
                shared_data[groupThreadIndex * 3 + 2] =
                    float3(data_SSBO[dataSizePoint + 2].xyz); //, data_SSBO[dataSizePoint + 7], data_SSBO[dataSizePoint + 8]);
                groupMemoryBarrier();
            #endif 
           
            uint maxJ = min(i + THREAD_COUNT, TRIANGLES);
            for(uint j = i; j < maxJ; ++j)
            {
                #if USE_LDS
                    float3 v1 = shared_data[j * 3 + 0].xyz;
                    float3 v2 = shared_data[j * 3 + 1].xyz;
                    float3 v3 = shared_data[j * 3 + 2].xyz;
                #else
                    uint dataSizePoint = (i + j) * ds;
                    float3 v1 = float3(data_SSBO[dataSizePoint + 0].xyz; //, data_SSBO[dataSizePoint + 1], data_SSBO[dataSizePoint + 2]);
                    float3 v2 = float3(data_SSBO[dataSizePoint + 1].xyz; //, data_SSBO[dataSizePoint + 4], data_SSBO[dataSizePoint + 5]);
                    float3 v3 = float3(data_SSBO[dataSizePoint + 2].xyz; //, data_SSBO[dataSizePoint + 7], data_SSBO[dataSizePoint + 8]);
                #endif
                float3 t1 = float3(v1.xy, 0.0f) - float3(vertexPos, 0.0f);
                float3 t2 = float3(v2.xy, 0.0f) - float3(vertexPos, 0.0f);
                float3 t3 = float3(v3.xy, 0.0f) - float3(vertexPos, 0.0f);

                float3 d1 = cross(t1, t2);
                float3 d2 = cross(t2, t3);
                float3 d3 = cross(t3, t1);

                if(d1.z >= 0.0f && d2.z >= 0.0f && d3.z >= 0.0f)
                {
                    maxv -= 0.01f;
                }
            }

        }
        float2 p = texCoord - float2(0.5f, 1.0f);
        float pdot = dot(p, p);
        //imageStore(storeTex0, storePos, float4(texCoord, data_SSBO[0], 1.0f));
        imageStore(storeTex0, int2(storePos), float4(texCoord, maxv, 1.0f));
    }
}
