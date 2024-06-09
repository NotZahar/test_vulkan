import subprocess
import os
from dataclasses import dataclass

@dataclass
class Shader:
    name: str
    path: str

sourcePath = os.path.join(os.getcwd(), 'src', 'shaders')
targetPath = os.path.join(os.getcwd(), 'build', 'shaders')
fragShaders = []
vertShaders = []

if not os.path.isdir(sourcePath):
    raise SystemExit('There is no source path')

if not os.path.isdir(targetPath):
    os.makedirs(targetPath)

with os.scandir(sourcePath) as shadersIt:
    for shader in shadersIt:
        if not shader.is_file():
            continue
        
        targetShader = Shader(shader.name, shader.path)
        if shader.name.endswith('frag'):
            fragShaders.append(targetShader)
            continue
        if shader.name.endswith('vert'):
            vertShaders.append(targetShader)
            continue

compileShader = lambda shader: subprocess.run(['glslc', shader.path, '-o', os.path.join(targetPath, shader.name + '.spv')])

for fragShader in fragShaders:
    compileShader(fragShader)

for vertShader in vertShaders:
    compileShader(vertShader)
