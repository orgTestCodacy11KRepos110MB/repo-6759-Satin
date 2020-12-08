//
//  MatCapMaterial.swift
//  Satin
//
//  Created by Reza Ali on 6/1/20.
//  MatCapMaterial inspired by @thespite
//  https://www.clicktorelease.com/code/spherical-normal-mapping/


import Metal
import simd

open class MatCapMaterial: BasicTextureMaterial {
    open override func compileSource() -> String? {
        return MatCapPipelineSource.setup(label: label)
    }
}

class MatCapPipelineSource {
    static let shared = MatCapPipelineSource()
    private static var sharedSource: String?

    class func setup(label: String) -> String? {
        guard MatCapPipelineSource.sharedSource == nil else { return sharedSource }
        do {
            MatCapPipelineSource.sharedSource = try compilePipelineSource(label)
        }
        catch {
            print(error)
        }
        return sharedSource
    }
}
