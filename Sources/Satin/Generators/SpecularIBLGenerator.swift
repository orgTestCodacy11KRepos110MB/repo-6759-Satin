//
//  SpecularIBLGenerator.swift
//  PBRTemplate
//
//  Created by Reza Ali on 11/8/22.
//  Copyright © 2022 Reza Ali. All rights reserved.
//

import Foundation
import Metal

public class SpecularIBLGenerator {
    class SpecularIBLComputeSystem: LiveTextureComputeSystem {
        var roughness: Float = 0.0
        var sourceTexture: MTLTexture?
        
        init(device: MTLDevice) {
            super.init(device: device, textureDescriptors: [], pipelinesURL: getPipelinesComputeUrl()!)
        }
        
        override func bind(_ computeEncoder: MTLComputeCommandEncoder) -> Int {
            let index = super.bind(computeEncoder)
            computeEncoder.setTexture(sourceTexture, index: index)
            return index + 1
        }
        
        override func bindUniforms(_ computeEncoder: MTLComputeCommandEncoder) {
            super.bindUniforms(computeEncoder)
            computeEncoder.setBytes(&roughness, length: MemoryLayout<Float>.size, index: ComputeBufferIndex.Custom0.rawValue)
        }
    }
    
    private var compute: SpecularIBLComputeSystem
    
    public init(device: MTLDevice) {
        self.compute = SpecularIBLComputeSystem(device: device)
    }
    
    public func encode(commandBuffer: MTLCommandBuffer, sourceTexture: MTLTexture, destinationTexture: MTLTexture) {
        let levels = destinationTexture.mipmapLevelCount
        var size = destinationTexture.width
        
        for level in 0..<levels {
            compute.roughness = Float(level) / Float(levels - 1)
            
            compute.sourceTexture = sourceTexture
            compute.textureDescriptors = Array(
                repeating: MTLTextureDescriptor.texture2DDescriptor(
                    pixelFormat: destinationTexture.pixelFormat,
                    width: size,
                    height: size,
                    mipmapped: false
                ),
                count: 6
            )
                        
            commandBuffer.label = "\(compute.label) Compute Command Buffer"
            
            compute.update(commandBuffer)
            
            commandBuffer.label = "\(compute.label) Blit Command Buffer"
            for slice in 0..<6 {
                if let blitEncoder = commandBuffer.makeBlitCommandEncoder() {
                    blitEncoder.copy(
                        from: compute.texture[slice],
                        sourceSlice: 0,
                        sourceLevel: 0,
                        to: destinationTexture,
                        destinationSlice: slice,
                        destinationLevel: level,
                        sliceCount: 1,
                        levelCount: 1
                    )
                    blitEncoder.endEncoding()
                }
            }
            
            size /= 2
        }
        
        destinationTexture.label = "Specular IBL"
    }
}
