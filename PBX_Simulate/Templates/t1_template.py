from abaqus import *
from abaqusConstants import *
from caeModules import *
import sys
import os

# === 当前工程 Abaqus 工作目录 ===
work_dir = '{{ABAQUS_WORK_DIR}}'
os.chdir(work_dir)

# === 设置基本参数 ===
job_name = 'Job-1'
cae_path = '{{CAE_FILE_PATH}}'

# === 打开模型数据库 ===
mdb = openMdb(cae_path)
mdb.Job(name=job_name, model='Model-1', description='', type=ANALYSIS, 
    atTime=None, waitMinutes=0, waitHours=0, queue=None, memory=90, 
    memoryUnits=PERCENTAGE, getMemoryFromAnalysis=True, 
    explicitPrecision=SINGLE, nodalOutputPrecision=SINGLE, echoPrint=OFF, 
    modelPrint=OFF, contactPrint=OFF, historyPrint=OFF, 
    userSubroutine='{{USER_SUBROUTINE_PATH}}', scratch='', 
    resultsFormat=ODB, numCpus=1, numGPUs=0)
mdb.jobs[job_name].submit()
mdb.jobs[job_name].waitForCompletion()
odb_path = os.path.join(os.getcwd(), job_name + '.odb')
if not os.path.isfile(odb_path):
    raise IOError('ODB error: %s' % odb_path)
o3 = session.openOdb(name=odb_path)
session.viewports['Viewport: 1'].setValues(displayedObject=o3)
session.viewports['Viewport: 1'].makeCurrent()
session.viewports['Viewport: 1'].odbDisplay.display.setValues(plotState=(
    CONTOURS_ON_DEF, ))
session.viewports['Viewport: 1'].view.setValues(nearPlane=454.433, 
    farPlane=578.295, width=274.793, height=145.597, cameraPosition=(-150.741, 
    512.077, 99.7923), cameraUpVector=(0.216627, -0.31919, -0.9226))
session.viewports['Viewport: 1'].view.setValues(nearPlane=449.359, 
    farPlane=587.609, width=271.725, height=143.971, cameraPosition=(-358.763, 
    372.968, 92.7705), cameraUpVector=(0.254567, -0.294106, -0.921247), 
    cameraTarget=(27.4205, 22.0013, 122.41))
session.viewports['Viewport: 1'].view.setValues(nearPlane=437.578, 
    farPlane=599.39, width=434.079, height=229.993, viewOffsetX=-4.05242, 
    viewOffsetY=-6.98482)
session.viewports['Viewport: 1'].view.setValues(nearPlane=436.113, 
    farPlane=601.767, width=432.626, height=229.223, cameraPosition=(-376.813, 
    352.403, 93.0223), cameraUpVector=(0.282054, -0.264816, -0.922127), 
    cameraTarget=(27.5109, 22.4833, 122.501), viewOffsetX=-4.03885, 
    viewOffsetY=-6.96144)
session.viewports['Viewport: 1'].view.setValues(nearPlane=429.026, 
    farPlane=608.854, width=545.113, height=288.823, viewOffsetX=-10.8007, 
    viewOffsetY=1.8316)
session.viewports['Viewport: 1'].view.setValues(nearPlane=427.265, 
    farPlane=610.906, width=542.875, height=287.637, cameraPosition=(-379.211, 
    349.721, 94.6533), cameraUpVector=(0.275971, -0.267688, -0.923138), 
    cameraTarget=(27.4766, 22.565, 122.367), viewOffsetX=-10.7564, 
    viewOffsetY=1.82408)
session.viewports['Viewport: 1'].view.setValues(nearPlane=436.338, 
    farPlane=613.314, width=554.402, height=293.745, cameraPosition=(-469.66, 
    194.731, 92.4661), cameraUpVector=(0.336796, -0.205838, -0.918803), 
    cameraTarget=(24.1311, 25.8791, 121.675), viewOffsetX=-10.9848, 
    viewOffsetY=1.86281)
session.viewports['Viewport: 1'].view.setValues(nearPlane=449.372, 
    farPlane=613.803, width=570.963, height=302.52, cameraPosition=(-501.476, 
    -25.5585, 104.675), cameraUpVector=(0.366627, 0.00380094, -0.93036), 
    cameraTarget=(18.3957, 25.5258, 122.483), viewOffsetX=-11.3129, 
    viewOffsetY=1.91845)
session.viewports['Viewport: 1'].view.setValues(nearPlane=439.928, 
    farPlane=632.227, width=558.964, height=296.162, cameraPosition=(-454.711, 
    -204.303, 77.1535), cameraUpVector=(0.365337, 0.19615, -0.909975), 
    cameraTarget=(14.6616, 21.0984, 122.722), viewOffsetX=-11.0751, 
    viewOffsetY=1.87813)
session.viewports['Viewport: 1'].view.setValues(nearPlane=433.575, 
    farPlane=641.306, width=550.891, height=291.885, cameraPosition=(-413.884, 
    -270.823, 40.6911), cameraUpVector=(0.393648, 0.266085, -0.879909), 
    cameraTarget=(13.8976, 18.4178, 121.532), viewOffsetX=-10.9152, 
    viewOffsetY=1.85101)
session.viewports['Viewport: 1'].view.setValues(nearPlane=428.909, 
    farPlane=647.661, width=544.962, height=288.744, cameraPosition=(-378.003, 
    -311.965, 11.4467), cameraUpVector=(0.415253, 0.317917, -0.852346), 
    cameraTarget=(13.7486, 16.4476, 120.391), viewOffsetX=-10.7977, 
    viewOffsetY=1.83109)
session.viewports['Viewport: 1'].view.setValues(nearPlane=388.432, 
    farPlane=683.785, width=493.533, height=261.495, cameraPosition=(-239.197, 
    -303.844, -213.943), cameraUpVector=(0.730474, 0.466249, -0.499019), 
    cameraTarget=(16.9198, 15.9382, 110.613), viewOffsetX=-9.77871, 
    viewOffsetY=1.65829)
leaf = dgo.LeafFromPartInstance(partInstanceName=("KE-1", ))
session.viewports['Viewport: 1'].odbDisplay.displayGroup.remove(leaf=leaf)
session.viewports['Viewport: 1'].animationController.setValues(
    animationType=SCALE_FACTOR)
session.viewports['Viewport: 1'].animationController.play(duration=UNLIMITED)
#: AVI Codec?????:Intel IYUV ?????????
session.aviOptions.setValues(compressionMethod=CODEC, 
    codecOptions='[12]:ejfjfffgbiaaaaaaaaaaaaaa')
session.imageAnimationOptions.setValues(vpDecorations=ON, vpBackground=OFF, 
    compass=OFF)
session.writeImageAnimation(fileName='{{RESULT_STRESS_PATH}}', 
    format=AVI, canvasObjects=(session.viewports['Viewport: 1'], ))
session.viewports['Viewport: 1'].odbDisplay.setPrimaryVariable(
    variableLabel='NT11', outputPosition=NODAL, )
session.imageAnimationOptions.setValues(vpDecorations=ON, vpBackground=OFF, 
    compass=OFF)
session.writeImageAnimation(fileName='{{RESULT_TEMP_PATH}}', format=AVI, 
    canvasObjects=(session.viewports['Viewport: 1'], ))
session.viewports['Viewport: 1'].odbDisplay.setPrimaryVariable(
    variableLabel='SDV1', outputPosition=INTEGRATION_POINT, )
session.imageAnimationOptions.setValues(vpDecorations=ON, vpBackground=OFF, 
    compass=OFF)
session.writeImageAnimation(fileName='{{RESULT_CURE_PATH}}', 
    format=AVI, canvasObjects=(session.viewports['Viewport: 1'], ))
