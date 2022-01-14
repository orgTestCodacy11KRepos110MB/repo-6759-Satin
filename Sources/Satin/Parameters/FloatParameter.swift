//
//  FloatParameter.swift
//  Satin
//
//  Created by Reza Ali on 10/22/19.
//  Copyright © 2019 Reza Ali. All rights reserved.
//

import Foundation

open class FloatParameter: NSObject, Parameter {
    public weak var delegate: ParameterDelegate?
    
    public static var type = ParameterType.float
    public var controlType: ControlType
    public let label: String
    public var string: String { return "float" }
    public var size: Int { return MemoryLayout<Float>.size }
    public var stride: Int { return MemoryLayout<Float>.stride }
    public var alignment: Int { return MemoryLayout<Float>.alignment }
    public var count: Int { return 1 }
    public var actions: [(Float) -> Void] = []
    
    public func onChange(_ fn: @escaping ((Float) -> ())) {
        actions.append(fn)
    }
    
    
    public subscript<Float>(index: Int) -> Float {
        get {
            return value as! Float
        }
        set {
            value = newValue as! Swift.Float
        }
    }
    
    public func dataType<Float>() -> Float.Type {
        return Float.self
    }
    
    private enum CodingKeys: String, CodingKey {
        case controlType
        case label
        case value
        case min
        case max
    }
    
    @objc public dynamic var value: Float {
        didSet {
            if oldValue != value {
                emit()
            }
        }
    }
    @objc public dynamic var min: Float
    @objc public dynamic var max: Float
    
    public init(_ label: String, _ value: Float, _ min: Float, _ max: Float, _ controlType: ControlType = .unknown, _ action: ((Float) -> Void)? = nil) {
        self.label = label
        self.controlType = controlType
        
        self.value = value
        self.min = min
        self.max = max
        
        if let a = action {
            actions.append(a)
        }
        super.init()
    }
    
    public init(_ label: String, _ value: Float = 0.0, _ controlType: ControlType = .unknown,
                _ action: ((Float) -> Void)? = nil) {
        self.label = label
        self.controlType = controlType
        
        self.value = value
        self.min = 0.0
        self.max = 1.0
        
        if let a = action {
            actions.append(a)
        }
        super.init()
    }
    
    public init(_ label: String, _ controlType: ControlType = .unknown, _ action: ((Float) -> Void)? = nil) {
        self.label = label
        self.controlType = controlType
        
        self.value = 0.0
        self.min = 0.0
        self.max = 1.0
        
        if let a = action {
            actions.append(a)
        }
        super.init()
    }
    
    public func alignData(pointer: UnsafeMutableRawPointer, offset: inout Int) -> UnsafeMutableRawPointer {
        var data = pointer
        let rem = offset % alignment
        if rem > 0 {
            let remOffset = alignment - rem
            data += remOffset
            offset += remOffset
        }
        return data
    }
    
    public func writeData(pointer: UnsafeMutableRawPointer, offset: inout Int) -> UnsafeMutableRawPointer {
        var data = alignData(pointer: pointer, offset: &offset)
        offset += size
        
        data.storeBytes(of: value, as: Float.self)
        data += size
        
        return data
    }
    
    func emit() {
        delegate?.updated(parameter: self)
        for action in self.actions {
            action(self.value)
        }
    }
    
    deinit {
        delegate = nil
        actions = []
    }
}