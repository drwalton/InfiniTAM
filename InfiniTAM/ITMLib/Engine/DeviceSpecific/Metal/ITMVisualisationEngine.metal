// Copyright 2014 Isis Innovation Limited and the authors of InfiniTAM

#include <metal_stdlib>

#include "../../DeviceAgnostic/ITMSceneReconstructionEngine.h"
#include "../../DeviceAgnostic/ITMVisualisationEngine.h"
#include "ITMVisualisationEngine_Metal.h"

using namespace metal;

kernel void createICPMaps_vh_device(DEVICEPTR(float) *depth                                        [[ buffer(0) ]],
                                    DEVICEPTR(Vector4f) *pointsRay                                 [[ buffer(1) ]],
                                    const DEVICEPTR(ITMVoxel) *voxelData                           [[ buffer(2) ]],
                                    const DEVICEPTR(typename ITMVoxelIndex::IndexData) *voxelIndex [[ buffer(3) ]],
                                    const DEVICEPTR(Vector2f) *minmaxdata                          [[ buffer(4) ]],
                                    const CONSTANT(CreateICPMaps_Params) *params                   [[ buffer(5) ]],
                                    uint2 threadIdx                                                [[ thread_position_in_threadgroup ]],
                                    uint2 blockIdx                                                 [[ threadgroup_position_in_grid ]],
                                    uint2 blockDim                                                 [[ threads_per_threadgroup ]])
{
    int x = threadIdx.x + blockIdx.x * blockDim.x, y = threadIdx.y + blockIdx.y * blockDim.y;
    
    if (x >= params->imgSize.x || y >= params->imgSize.y) return;
    
    int locId = x + y * params->imgSize.x;
    int locId2 = (int)floor((float)x / minmaximg_subsample) + (int)floor((float)y / minmaximg_subsample) * params->imgSize.x;
    
    castRay<ITMVoxel, ITMVoxelIndex>(pointsRay[locId], x, y, voxelData, voxelIndex, params->invM, params->projParams,
                                     params->voxelSizes.y, params->lightSource.w, minmaxdata[locId2]);
}

kernel void createICPMaps_vh_normals_device(DEVICEPTR(Vector4f) *pointsRay                                 [[ buffer(0) ]],
                                            DEVICEPTR(Vector4f) *pointsMap                                 [[ buffer(1) ]],
                                            DEVICEPTR(Vector4f) *normalsMap                                [[ buffer(2) ]],
                                            DEVICEPTR(Vector4u) *outRendering                              [[ buffer(3) ]],
                                            const DEVICEPTR(ITMVoxel) *voxelData                           [[ buffer(4) ]],
                                            const DEVICEPTR(typename ITMVoxelIndex::IndexData) *voxelIndex [[ buffer(5) ]],
                                            const CONSTANT(CreateICPMaps_Params) *params                   [[ buffer(6) ]],
                                            uint2 threadIdx                                                [[ thread_position_in_threadgroup ]],
                                            uint2 blockIdx                                                 [[ threadgroup_position_in_grid ]],
                                            uint2 blockDim                                                 [[ threads_per_threadgroup ]])
{
    int x = threadIdx.x + blockIdx.x * blockDim.x, y = threadIdx.y + blockIdx.y * blockDim.y;
    
    if (x >= params->imgSize.x || y >= params->imgSize.y) return;
    
    int locId = x + y * params->imgSize.x;
    
    Vector4f ptRay = pointsRay[locId];
    processPixelICP<ITMVoxel, ITMVoxelIndex>(outRendering[locId], pointsMap[locId], normalsMap[locId], TO_VECTOR3(ptRay), ptRay.w > 0, voxelData,
                                             voxelIndex, params->voxelSizes.x, TO_VECTOR3(params->lightSource));
}