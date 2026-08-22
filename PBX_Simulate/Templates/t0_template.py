# -*- coding: utf-8 -*-

from abaqus import *
from abaqusConstants import *
from caeModules import *
from driverUtils import executeOnCaeStartup
executeOnCaeStartup()
Mdb()
model = mdb.models['Model-1']

# =========================================================
# 1. 参数
# =========================================================
r = {{CHARGE_RADIUS}}
h = {{CHARGE_HEIGHT}}

kt = {{SHELL_THICKNESS}}
kh = h



# =========================================================
# 2. 几何（保持你原来的）
# =========================================================
def revolve_part(name, lines, sheet=2000):
    sk = model.ConstrainedSketch(name='__profile__', sheetSize=sheet)
    sk.ConstructionLine(point1=(0,-sheet), point2=(0,sheet))
    sk.assignCenterline(sk.geometry[2])
    for l in lines:
        sk.Line(point1=l[0], point2=l[1])
    p = model.Part(name=name, dimensionality=THREE_D, type=DEFORMABLE_BODY)
    p.BaseSolidRevolve(sketch=sk, angle=90.0)
    return p

p_ke = revolve_part('ke', [
    ((0,0),(r+kt,0)),
    ((r+kt,0),(r+kt,h+kt)),
    ((r+kt,h+kt),(r,h+kt)),
    ((r,h+kt),(r,kt)),
    ((r,kt),(0,kt)),
    ((0,kt),(0,0))
])

p_yao = revolve_part('yao', [
    ((0,kt),(r,kt)),
    ((r,kt),(r,h+kt)),
    ((r,h+kt),(0,h+kt)),
    ((0,h+kt),(0,kt))
])


# =========================================================
# 3. 材料与截面
# =========================================================
mdb.models['Model-1'].Material(name='gang')
mdb.models['Model-1'].materials['gang'].Density(table=(({{MOLD_DENSITY}}, ), ))
mdb.models['Model-1'].materials['gang'].Elastic(table=((
    {{MOLD_ELASTIC_MODULUS}},
    {{MOLD_POISSON_RATIO}}
), ))
mdb.models['Model-1'].materials['gang'].Conductivity(table=(({{MOLD_THERMAL_CONDUCTIVITY}}, ), ))
mdb.models['Model-1'].materials['gang'].SpecificHeat(table=(({{MOLD_SPECIFIC_HEAT}}, ), ))
mdb.models['Model-1'].Material(name='pbx')
mdb.models['Model-1'].materials['pbx'].Density(table=(({{PBX_DENSITY}}, ), ))
mdb.models['Model-1'].materials['pbx'].Depvar(n=4)
mdb.models['Model-1'].materials['pbx'].UserDefinedField()
mdb.models['Model-1'].materials['pbx'].Elastic(dependencies=1, table=(
    (
        {{PBX_INITIAL_ELASTIC_MODULUS}},
        {{PBX_INITIAL_POISSON_RATIO}},
        0.0
    ),
    (
        {{PBX_FINAL_ELASTIC_MODULUS}},
        {{PBX_FINAL_POISSON_RATIO}},
        1.0
    )
))
mdb.models['Model-1'].materials['pbx'].Plastic(scaleStress=None, table=(({{PBX_YIELD_STRESS}}, 0.0), ))
mdb.models['Model-1'].materials['pbx'].Expansion(table=(({{PBX_EXPANSION_COEFFICIENT}}, ), ))
mdb.models['Model-1'].materials['pbx'].Conductivity(table=(({{PBX_THERMAL_CONDUCTIVITY}}, ), ))
mdb.models['Model-1'].materials['pbx'].HeatGeneration()
mdb.models['Model-1'].materials['pbx'].SpecificHeat(table=(({{PBX_SPECIFIC_HEAT}}, ), ))
mdb.models['Model-1'].HomogeneousSolidSection(name='ke', material='gang', 
    thickness=None)
mdb.models['Model-1'].HomogeneousSolidSection(name='yao', material='pbx', 
    thickness=None)
p = mdb.models['Model-1'].parts['yao']
c = p.cells
cells = c.getSequenceFromMask(mask=('[#1 ]', ), )
region = p.Set(cells=cells, name='Set-1')
p = mdb.models['Model-1'].parts['yao']
p.SectionAssignment(region=region, sectionName='yao', offset=0.0, 
    offsetType=MIDDLE_SURFACE, offsetField='', 
    thicknessAssignment=FROM_SECTION)
p = mdb.models['Model-1'].parts['ke']
session.viewports['Viewport: 1'].setValues(displayedObject=p)
session.viewports['Viewport: 1'].view.setValues(nearPlane=374.564, 
    farPlane=604.052, width=317.437, height=171.144, viewOffsetX=27.2925, 
    viewOffsetY=-2.89917)
p = mdb.models['Model-1'].parts['ke']
c = p.cells
cells = c.getSequenceFromMask(mask=('[#1 ]', ), )
region = p.Set(cells=cells, name='Set-1')
p = mdb.models['Model-1'].parts['ke']
p.SectionAssignment(region=region, sectionName='ke', offset=0.0, 
    offsetType=MIDDLE_SURFACE, offsetField='', 
    thicknessAssignment=FROM_SECTION)
a = mdb.models['Model-1'].rootAssembly
session.viewports['Viewport: 1'].setValues(displayedObject=a)
session.viewports['Viewport: 1'].assemblyDisplay.setValues(
    optimizationTasks=OFF, geometricRestrictions=OFF, stopConditions=OFF)
a = mdb.models['Model-1'].rootAssembly
a.DatumCsysByDefault(CARTESIAN)
p = mdb.models['Model-1'].parts['ke']
a.Instance(name='ke-1', part=p, dependent=OFF)
p = mdb.models['Model-1'].parts['yao']
a.Instance(name='yao-1', part=p, dependent=OFF)
mdb.models['Model-1'].CoupledTempDisplacementStep(name='Step-1', 
    previous='Initial', timePeriod={{SIMULATION_TIME_LENGTH}}, maxNumInc=1000000000, 
    initialInc=0.0001, minInc=1e-05, maxInc=100000.0, deltmx=0.1)
session.viewports['Viewport: 1'].assemblyDisplay.setValues(step='Step-1')
mdb.models['Model-1'].fieldOutputRequests['F-Output-1'].setValues(variables=(
    'S', 'E', 'NT', 'SDV'))
mdb.models['Model-1'].FieldOutputRequest(name='F-Output-2', 
    createStepName='Step-1', variables=('SDV', ))
mdb.models['Model-1'].fieldOutputRequests['F-Output-1'].setValues(frequency=10)
mdb.models['Model-1'].fieldOutputRequests['F-Output-2'].setValues(frequency=10)
session.viewports['Viewport: 1'].assemblyDisplay.setValues(interactions=ON, 
    constraints=ON, connectors=ON, engineeringFeatures=ON, 
    adaptiveMeshConstraints=OFF)
mdb.models['Model-1'].ContactProperty('IntProp-1')
mdb.models['Model-1'].interactionProperties['IntProp-1'].ThermalConductance(
    definition=TABULAR, clearanceDependency=ON, pressureDependency=OFF, 
    temperatureDependencyC=OFF, massFlowRateDependencyC=OFF, dependenciesC=0, 
    clearanceDepTable=((1000.0, 0.0), (0.0, 0.1)))
mdb.models['Model-1'].interactionProperties['IntProp-1'].GeometricProperties(
    contactArea=1.0, padThickness=None)
#: 相互作用属性 "IntProp-1" 已创建.
session.viewports['Viewport: 1'].view.setValues(nearPlane=372.939, 
    farPlane=665.962, width=221.624, height=119.487, cameraPosition=(-27.0447, 
    278.656, -338.934), cameraUpVector=(0.00987852, -0.984754, -0.173674), 
    cameraTarget=(29.5453, 22.563, 113.181))
a = mdb.models['Model-1'].rootAssembly
s1 = a.instances['ke-1'].faces
side1Faces1 = s1.getSequenceFromMask(mask=('[#49 ]', ), )
region=a.Surface(side1Faces=side1Faces1, name='Surf-1')
mdb.models['Model-1'].FilmCondition(name='Int-1', createStepName='Step-1', 
    surface=region, definition=USER_SUB, filmCoeff=10.0, sinkTemperature=1.0, 
    sinkDistributionType=UNIFORM, sinkFieldName='')
#: 相互作用 "Int-1" 已创建.
a = mdb.models['Model-1'].rootAssembly
s1 = a.instances['yao-1'].faces
side1Faces1 = s1.getSequenceFromMask(mask=('[#10 ]', ), )
region=a.Surface(side1Faces=side1Faces1, name='Surf-2')
mdb.models['Model-1'].FilmCondition(name='Int-2', createStepName='Step-1', 
    surface=region, definition=USER_SUB, filmCoeff=8.0, sinkTemperature=1.0, 
    sinkDistributionType=UNIFORM, sinkFieldName='')
#: 相互作用 "Int-2" 已创建.
i1 = mdb.models['Model-1'].rootAssembly.allInstances['yao-1']
leaf = dgm.LeafFromInstance(instances=(i1, ))
session.viewports['Viewport: 1'].assemblyDisplay.displayGroup.replace(
    leaf=leaf)
session.viewports['Viewport: 1'].view.setValues(nearPlane=439.691, 
    farPlane=592.421, width=261.293, height=140.874, cameraPosition=(-230.01, 
    469.952, 52.4318), cameraUpVector=(-0.397979, -0.695996, -0.597664), 
    cameraTarget=(31.2833, 20.9249, 109.83))
session.viewports['Viewport: 1'].view.setValues(nearPlane=378.534, 
    farPlane=653.321, width=224.95, height=121.28, cameraPosition=(-60.1882, 
    360.709, 492.408), cameraUpVector=(-0.863276, -0.223436, -0.452582), 
    cameraTarget=(29.1036, 22.3271, 104.183))
i1 = mdb.models['Model-1'].rootAssembly.allInstances['ke-1']
leaf = dgm.LeafFromInstance(instances=(i1, ))
session.viewports['Viewport: 1'].assemblyDisplay.displayGroup.replace(
    leaf=leaf)
session.viewports['Viewport: 1'].view.setValues(nearPlane=387.88, 
    farPlane=636.877, width=230.504, height=124.274, cameraPosition=(-368.878, 
    71.2606, 434.218), cameraUpVector=(-0.254092, -0.593967, -0.763309), 
    cameraTarget=(35.0023, 27.8581, 105.295))
session.viewports['Viewport: 1'].view.setValues(nearPlane=375.505, 
    farPlane=651.475, width=223.15, height=120.31, cameraPosition=(-296.094, 
    151.344, -266.475), cameraUpVector=(0.351102, -0.924543, -0.14815), 
    cameraTarget=(33.5389, 26.248, 119.383))
a = mdb.models['Model-1'].rootAssembly
s1 = a.instances['yao-1'].faces
side1Faces1 = s1.getSequenceFromMask(mask=('[#9 ]', ), )
region1=a.Surface(side1Faces=side1Faces1, name='m_Surf-3')
a = mdb.models['Model-1'].rootAssembly
s1 = a.instances['ke-1'].faces
side1Faces1 = s1.getSequenceFromMask(mask=('[#22 ]', ), )
region2=a.Surface(side1Faces=side1Faces1, name='s_Surf-3')
mdb.models['Model-1'].SurfaceToSurfaceContactStd(name='Int-3', 
    createStepName='Step-1', main=region1, secondary=region2, sliding=SMALL, 
    thickness=ON, interactionProperty='IntProp-1', adjustMethod=NONE, 
    initialClearance=OMIT, datumAxis=None, clearanceRegion=None)
#: 相互作用 "Int-3" 已创建.
session.viewports['Viewport: 1'].assemblyDisplay.setValues(loads=ON, bcs=ON, 
    predefinedFields=ON, interactions=OFF, constraints=OFF, 
    engineeringFeatures=OFF)
i1 = mdb.models['Model-1'].rootAssembly.allInstances['yao-1']
leaf = dgm.LeafFromInstance(instances=(i1, ))
session.viewports['Viewport: 1'].assemblyDisplay.displayGroup.replace(
    leaf=leaf)
session.viewports['Viewport: 1'].view.setValues(nearPlane=436.034, 
    farPlane=587.307, width=259.121, height=139.703, cameraPosition=(-423.449, 
    265.348, 55.6845), cameraUpVector=(-0.0429197, -0.901973, -0.429653), 
    cameraTarget=(36.5349, 23.5661, 111.804))
session.viewports['Viewport: 1'].view.setValues(nearPlane=380.815, 
    farPlane=638.818, width=226.306, height=122.011, cameraPosition=(-347.204, 
    -97.2068, 436.276), cameraUpVector=(0.169414, -0.762849, -0.623987), 
    cameraTarget=(34.8945, 31.3663, 103.616))
a = mdb.models['Model-1'].rootAssembly
f1 = a.instances['yao-1'].faces
faces1 = f1.getSequenceFromMask(mask=('[#e ]', ), )
region = a.Set(faces=faces1, name='Set-1')
mdb.models['Model-1'].DisplacementBC(name='BC-1', createStepName='Step-1', 
    region=region, u1=0.0, u2=0.0, u3=0.0, ur1=UNSET, ur2=UNSET, ur3=UNSET, 
    amplitude=UNSET, fixed=OFF, distributionType=UNIFORM, fieldName='', 
    localCsys=None)
i1 = mdb.models['Model-1'].rootAssembly.allInstances['ke-1']
leaf = dgm.LeafFromInstance(instances=(i1, ))
session.viewports['Viewport: 1'].assemblyDisplay.displayGroup.replace(
    leaf=leaf)
session.viewports['Viewport: 1'].view.setValues(nearPlane=362.773, 
    farPlane=657.626, width=215.584, height=116.231, cameraPosition=(-70.7918, 
    238.143, 566.676), cameraUpVector=(-0.761846, -0.591814, -0.263338), 
    cameraTarget=(28.3933, 23.4789, 100.549))
session.viewports['Viewport: 1'].view.setValues(nearPlane=364.042, 
    farPlane=659.372, width=216.34, height=116.638, cameraPosition=(-125.132, 
    178.102, -352.117), cameraUpVector=(-0.535481, -0.795027, 0.284941), 
    cameraTarget=(29.7224, 24.9474, 123.022))
a = mdb.models['Model-1'].rootAssembly
f1 = a.instances['ke-1'].faces
faces1 = f1.getSequenceFromMask(mask=('[#49 ]', ), )
region = a.Set(faces=faces1, name='Set-2')
mdb.models['Model-1'].TemperatureBC(name='BC-2', createStepName='Step-1', 
    region=region, fixed=OFF, distributionType=USER_DEFINED, fieldName='', 
    magnitude={{AMBIENT_TEMPERATURE}}, amplitude=UNSET)
a = mdb.models['Model-1'].rootAssembly
f1 = a.instances['ke-1'].faces
faces1 = f1.getSequenceFromMask(mask=('[#22 ]', ), )
region = a.Set(faces=faces1, name='Set-3')
mdb.models['Model-1'].EncastreBC(name='BC-3', createStepName='Step-1', 
    region=region, localCsys=None)
session.viewports['Viewport: 1'].assemblyDisplay.setValues(step='Initial')
i1 = mdb.models['Model-1'].rootAssembly.allInstances['ke-1']
i2 = mdb.models['Model-1'].rootAssembly.allInstances['yao-1']
leaf = dgm.LeafFromInstance(instances=(i1, i2, ))
session.viewports['Viewport: 1'].assemblyDisplay.displayGroup.replace(
    leaf=leaf)
a = mdb.models['Model-1'].rootAssembly
c1 = a.instances['ke-1'].cells
cells1 = c1.getSequenceFromMask(mask=('[#1 ]', ), )
f1 = a.instances['ke-1'].faces
faces1 = f1.getSequenceFromMask(mask=('[#7f ]', ), )
e1 = a.instances['ke-1'].edges
edges1 = e1.getSequenceFromMask(mask=('[#1ffff ]', ), )
v1 = a.instances['ke-1'].vertices
verts1 = v1.getSequenceFromMask(mask=('[#fff ]', ), )
c2 = a.instances['yao-1'].cells
cells2 = c2.getSequenceFromMask(mask=('[#1 ]', ), )
f2 = a.instances['yao-1'].faces
faces2 = f2.getSequenceFromMask(mask=('[#1f ]', ), )
e2 = a.instances['yao-1'].edges
edges2 = e2.getSequenceFromMask(mask=('[#1ff ]', ), )
v2 = a.instances['yao-1'].vertices
verts2 = v2.getSequenceFromMask(mask=('[#3f ]', ), )
region = a.Set(vertices=verts1+verts2, edges=edges1+edges2, faces=faces1+\
    faces2, cells=cells1+cells2, name='Set-4')
mdb.models['Model-1'].Temperature(name='Predefined Field-1', 
    createStepName='Initial', region=region, distributionType=UNIFORM, 
    crossSectionDistribution=CONSTANT_THROUGH_THICKNESS, magnitudes=({{AMBIENT_TEMPERATURE}}, ))
session.viewports['Viewport: 1'].assemblyDisplay.setValues(mesh=ON, loads=OFF, 
    bcs=OFF, predefinedFields=OFF, connectors=OFF)
session.viewports['Viewport: 1'].assemblyDisplay.meshOptions.setValues(
    meshTechnique=ON)
i1 = mdb.models['Model-1'].rootAssembly.allInstances['ke-1']
leaf = dgm.LeafFromInstance(instances=(i1, ))
session.viewports['Viewport: 1'].assemblyDisplay.displayGroup.replace(
    leaf=leaf)
a = mdb.models['Model-1'].rootAssembly
partInstances =(a.instances['ke-1'], )
a.seedPartInstance(regions=partInstances, size=6.0, deviationFactor=0.1, 
    minSizeFactor=0.1)
a = mdb.models['Model-1'].rootAssembly
c1 = a.instances['ke-1'].cells
pickedRegions = c1.getSequenceFromMask(mask=('[#1 ]', ), )
a.setMeshControls(regions=pickedRegions, elemShape=HEX_DOMINATED, 
    technique=SWEEP, algorithm=ADVANCING_FRONT)
a = mdb.models['Model-1'].rootAssembly
partInstances =(a.instances['ke-1'], )
a.generateMesh(regions=partInstances)
elemType1 = mesh.ElemType(elemCode=C3D8T, elemLibrary=STANDARD, 
    secondOrderAccuracy=OFF, distortionControl=DEFAULT)
elemType2 = mesh.ElemType(elemCode=C3D6T, elemLibrary=STANDARD)
elemType3 = mesh.ElemType(elemCode=C3D4T, elemLibrary=STANDARD)
a = mdb.models['Model-1'].rootAssembly
c1 = a.instances['ke-1'].cells
cells1 = c1.getSequenceFromMask(mask=('[#1 ]', ), )
pickedRegions =(cells1, )
a.setElementType(regions=pickedRegions, elemTypes=(elemType1, elemType2, 
    elemType3))
i1 = mdb.models['Model-1'].rootAssembly.allInstances['yao-1']
leaf = dgm.LeafFromInstance(instances=(i1, ))
session.viewports['Viewport: 1'].assemblyDisplay.displayGroup.replace(
    leaf=leaf)
a = mdb.models['Model-1'].rootAssembly
partInstances =(a.instances['yao-1'], )
a.seedPartInstance(regions=partInstances, size=4.0, deviationFactor=0.1, 
    minSizeFactor=0.1)
a = mdb.models['Model-1'].rootAssembly
c1 = a.instances['yao-1'].cells
pickedRegions = c1.getSequenceFromMask(mask=('[#1 ]', ), )
a.setMeshControls(regions=pickedRegions, elemShape=HEX_DOMINATED)
a = mdb.models['Model-1'].rootAssembly
partInstances =(a.instances['yao-1'], )
a.generateMesh(regions=partInstances)
elemType1 = mesh.ElemType(elemCode=C3D8T, elemLibrary=STANDARD, 
    secondOrderAccuracy=OFF, distortionControl=DEFAULT)
elemType2 = mesh.ElemType(elemCode=C3D6T, elemLibrary=STANDARD)
elemType3 = mesh.ElemType(elemCode=C3D4T, elemLibrary=STANDARD)
a = mdb.models['Model-1'].rootAssembly
c1 = a.instances['yao-1'].cells
cells1 = c1.getSequenceFromMask(mask=('[#1 ]', ), )
pickedRegions =(cells1, )
a.setElementType(regions=pickedRegions, elemTypes=(elemType1, elemType2, 
    elemType3))
i1 = mdb.models['Model-1'].rootAssembly.allInstances['ke-1']
i2 = mdb.models['Model-1'].rootAssembly.allInstances['yao-1']
leaf = dgm.LeafFromInstance(instances=(i1, i2, ))
session.viewports['Viewport: 1'].assemblyDisplay.displayGroup.replace(
    leaf=leaf)
session.viewports['Viewport: 1'].view.setValues(nearPlane=395.939, 
    farPlane=628.712, width=235.295, height=127.222, cameraPosition=(-153.981, 
    434.807, -139.777), cameraUpVector=(-0.379965, -0.835225, -0.397524), 
    cameraTarget=(30.341, 19.4434, 118.469))
session.viewports['Viewport: 1'].view.setValues(nearPlane=380.241, 
    farPlane=644.409, width=351.315, height=189.952, viewOffsetX=-0.269756, 
    viewOffsetY=-12.3604)
session.viewports['Viewport: 1'].view.setValues(nearPlane=411.88, 
    farPlane=613.298, width=380.547, height=205.758, cameraPosition=(-337.913, 
    382.125, 53.0961), cameraUpVector=(-0.393633, -0.907192, -0.148515), 
    cameraTarget=(31.4852, 18.6545, 121.12), viewOffsetX=-0.292202, 
    viewOffsetY=-13.3889)
session.viewports['Viewport: 1'].assemblyDisplay.setValues(mesh=OFF)
session.viewports['Viewport: 1'].assemblyDisplay.meshOptions.setValues(
    meshTechnique=OFF)
session.viewports['Viewport: 1'].view.setValues(nearPlane=259.566, 
    farPlane=625.27, width=513.971, height=277.103, viewOffsetX=-50.3937, 
    viewOffsetY=12.2586)
session.viewports['Viewport: 1'].view.setValues(nearPlane=257.112, 
    farPlane=627.724, width=509.113, height=274.484, viewOffsetX=77.543, 
    viewOffsetY=-20.897)
session.viewports['Viewport: 1'].assemblyDisplay.setValues(interactions=ON, 
    constraints=ON, connectors=ON, engineeringFeatures=ON)
del mdb.models['Model-1'].interactions['Int-1']
del mdb.models['Model-1'].interactions['Int-2']
del mdb.models['Model-1'].interactions['Int-3']
session.viewports['Viewport: 1'].assemblyDisplay.setValues(step='Step-1')
session.viewports['Viewport: 1'].view.setValues(nearPlane=269.377, 
    farPlane=612.61, width=533.399, height=287.578, cameraPosition=(368.944, 
    389.275, 132.716), cameraUpVector=(-0.82856, 0.554573, 0.0770567), 
    cameraTarget=(-50.6929, 77.8182, 123.099), viewOffsetX=81.242, 
    viewOffsetY=-21.8938)
session.viewports['Viewport: 1'].view.setValues(nearPlane=218.952, 
    farPlane=626.674, width=433.551, height=233.746, cameraPosition=(145.44, 
    433.092, 297.503), cameraUpVector=(-0.967986, 0.168737, -0.185824), 
    cameraTarget=(-64.8239, 11.0516, 71.9735), viewOffsetX=66.0342, 
    viewOffsetY=-17.7955)
session.viewports['Viewport: 1'].view.setValues(nearPlane=263.598, 
    farPlane=553.09, width=521.957, height=281.409, cameraPosition=(224.534, 
    -54.4713, 361.482), cameraUpVector=(-0.352771, 0.880872, 0.315621), 
    cameraTarget=(-128.648, 124.191, 20.1103), viewOffsetX=79.4992, 
    viewOffsetY=-21.4242)
session.viewports['Viewport: 1'].view.setValues(nearPlane=217.483, 
    farPlane=597.149, width=430.643, height=232.178, cameraPosition=(164.84, 
    -185.588, 291.522), cameraUpVector=(-0.122579, 0.816607, 0.564028), 
    cameraTarget=(-120.89, 165.432, 30.1108), viewOffsetX=65.5912, 
    viewOffsetY=-17.6761)
session.viewports['Viewport: 1'].view.setValues(nearPlane=213.647, 
    farPlane=613.645, width=423.048, height=228.083, cameraPosition=(106.659, 
    522.312, 125.807), cameraUpVector=(-0.986976, -0.128206, 0.0971688), 
    cameraTarget=(-1.69948, 11.3123, 143.99), viewOffsetX=64.4343, 
    viewOffsetY=-17.3643)
session.viewports['Viewport: 1'].view.setValues(nearPlane=293.393, 
    farPlane=538.156, width=580.955, height=313.217, cameraPosition=(268.015, 
    -21.4518, 358.736), cameraUpVector=(-0.518194, 0.789106, 0.329828), 
    cameraTarget=(-124.166, 79.8715, 28.4016), viewOffsetX=88.4851, 
    viewOffsetY=-23.8457)
session.viewports['Viewport: 1'].view.setValues(nearPlane=207.332, 
    farPlane=630.286, width=410.543, height=221.341, cameraPosition=(124.057, 
    -268.888, 209.844), cameraUpVector=(-0.0683422, 0.659631, 0.748476), 
    cameraTarget=(-116.406, 163.009, 40.0326), viewOffsetX=62.5298, 
    viewOffsetY=-16.851)
session.viewports['Viewport: 1'].view.setValues(nearPlane=233.556, 
    farPlane=596.241, width=462.47, height=249.337, cameraPosition=(175.623, 
    357.06, 347.745), cameraUpVector=(-0.932108, 0.31778, -0.173754), 
    cameraTarget=(-77.6455, 3.75587, 57.5303), viewOffsetX=70.4388, 
    viewOffsetY=-18.9824)
session.viewports['Viewport: 1'].view.setValues(nearPlane=273.595, 
    farPlane=544.959, width=541.752, height=292.081, cameraPosition=(138.759, 
    165.789, 433.384), cameraUpVector=(-0.837198, 0.532732, -0.123679), 
    cameraTarget=(-96.5015, 20.1672, -10.057), viewOffsetX=82.5143, 
    viewOffsetY=-22.2366)
session.viewports['Viewport: 1'].view.setValues(nearPlane=193.613, 
    farPlane=620.257, width=383.377, height=206.694, cameraPosition=(-140.42, 
    469.861, 175.942), cameraUpVector=(-0.732044, -0.445233, -0.515635), 
    cameraTarget=(-13.5552, -29.736, 89.3298), viewOffsetX=58.3922, 
    viewOffsetY=-15.736)
a = mdb.models['Model-1'].rootAssembly
s1 = a.instances['ke-1'].faces
side1Faces1 = s1.getSequenceFromMask(mask=('[#1c ]', ), )
region=a.Surface(side1Faces=side1Faces1, name='Surf-5')
mdb.models['Model-1'].FilmCondition(name='Int-1', createStepName='Step-1', 
    surface=region, definition=USER_SUB, filmCoeff=10.0, sinkTemperature=1.0, 
    sinkDistributionType=UNIFORM, sinkFieldName='')
#: 相互作用 "Int-1" 已创建.
session.viewports['Viewport: 1'].view.setValues(nearPlane=238.992, 
    farPlane=560.188, width=473.234, height=255.14, cameraPosition=(-189.22, 
    313.105, 319.948), cameraUpVector=(-0.217022, 0.498736, -0.839145), 
    cameraTarget=(-49.8158, 26.3057, -94.185), viewOffsetX=72.0783, 
    viewOffsetY=-19.4242)
session.viewports['Viewport: 1'].view.setValues(nearPlane=230.387, 
    farPlane=571.457, width=456.194, height=245.953, cameraPosition=(-154.635, 
    341.699, 324.428), cameraUpVector=(-0.221528, 0.488107, -0.844202), 
    cameraTarget=(-64.2883, 30.4361, -85.6292), viewOffsetX=69.483, 
    viewOffsetY=-18.7248)
session.viewports['Viewport: 1'].view.setValues(nearPlane=219.35, 
    farPlane=593.242, width=434.34, height=234.171, cameraPosition=(-21.6417, 
    431.033, 300.103), cameraUpVector=(-0.447147, 0.323999, -0.833717), 
    cameraTarget=(-102.828, 27.5792, -22.1209), viewOffsetX=66.1544, 
    viewOffsetY=-17.8278)
session.viewports['Viewport: 1'].view.setValues(nearPlane=258.951, 
    farPlane=550.4, width=512.754, height=276.447, cameraPosition=(35.1873, 
    297.515, 405.252), cameraUpVector=(-0.317839, 0.722299, -0.614217), 
    cameraTarget=(-116.51, 86.3866, -48.1869), viewOffsetX=78.0976, 
    viewOffsetY=-21.0464)
a = mdb.models['Model-1'].rootAssembly
s1 = a.instances['yao-1'].faces
side1Faces1 = s1.getSequenceFromMask(mask=('[#1 ]', ), )
region=a.Surface(side1Faces=side1Faces1, name='Surf-6')
mdb.models['Model-1'].FilmCondition(name='Int-2', createStepName='Step-1', 
    surface=region, definition=USER_SUB, filmCoeff=8.0, sinkTemperature=1.0, 
    sinkDistributionType=UNIFORM, sinkFieldName='')
#: 相互作用 "Int-2" 已创建.
session.viewports['Viewport: 1'].view.setValues(nearPlane=257.865, 
    farPlane=525.153, width=510.603, height=275.288, cameraPosition=(-352.656, 
    -11.6831, 122.221), cameraUpVector=(-0.14407, 0.845462, -0.514235), 
    cameraTarget=(80.6768, 105.342, -145.589), viewOffsetX=77.77, 
    viewOffsetY=-20.9581)
session.viewports['Viewport: 1'].view.setValues(nearPlane=227.495, 
    farPlane=556.415, width=450.467, height=242.866, cameraPosition=(-345.981, 
    283.346, 52.3286), cameraUpVector=(0.504997, 0.609141, -0.611494), 
    cameraTarget=(77.5269, 36.362, -128.875), viewOffsetX=68.6106, 
    viewOffsetY=-18.4898)
session.viewports['Viewport: 1'].view.setValues(nearPlane=253.503, 
    farPlane=528.479, width=501.966, height=270.631, cameraPosition=(-313.131, 
    261.891, 201.091), cameraUpVector=(0.443784, 0.803113, -0.397575), 
    cameraTarget=(38.8801, 108.719, -153.622), viewOffsetX=76.4544, 
    viewOffsetY=-20.6036)
session.viewports['Viewport: 1'].view.setValues(nearPlane=209.39, 
    farPlane=576.502, width=414.617, height=223.537, cameraPosition=(-221.363, 
    379.924, -162.86), cameraUpVector=(0.892557, 0.450928, 0.00246046), 
    cameraTarget=(152.076, 26.6086, -68.4865), viewOffsetX=63.1503, 
    viewOffsetY=-17.0183)
session.viewports['Viewport: 1'].view.setValues(nearPlane=239.319, 
    farPlane=547.469, width=473.88, height=255.488, cameraPosition=(-235.257, 
    305.109, -225.217), cameraUpVector=(0.774765, 0.627531, 0.0770965), 
    cameraTarget=(179.879, 47.4096, -39.6141), viewOffsetX=72.1767, 
    viewOffsetY=-19.4508)
i1 = mdb.models['Model-1'].rootAssembly.allInstances['ke-1']
leaf = dgm.LeafFromInstance(instances=(i1, ))
session.viewports['Viewport: 1'].assemblyDisplay.displayGroup.replace(
    leaf=leaf)
session.viewports['Viewport: 1'].view.setValues(nearPlane=247.14, 
    farPlane=540.438, width=489.366, height=263.838, cameraPosition=(-81.8092, 
    214.876, -357.074), cameraUpVector=(0.784318, 0.587466, 0.199321), 
    cameraTarget=(179.908, 23.0138, 52.6678), viewOffsetX=74.5354, 
    viewOffsetY=-20.0865)
i1 = mdb.models['Model-1'].rootAssembly.allInstances['yao-1']
leaf = dgm.LeafFromInstance(instances=(i1, ))
session.viewports['Viewport: 1'].assemblyDisplay.displayGroup.replace(
    leaf=leaf)
session.viewports['Viewport: 1'].view.setValues(nearPlane=216.929, 
    farPlane=560.449, width=429.545, height=231.586, cameraPosition=(-106.068, 
    303.626, -312.463), cameraUpVector=(0.908638, 0.385565, 0.160362), 
    cameraTarget=(160.418, -7.51927, 12.1435), viewOffsetX=65.424, 
    viewOffsetY=-17.6311)
session.viewports['Viewport: 1'].view.setValues(nearPlane=284.106, 
    farPlane=498.163, width=562.563, height=303.302, cameraPosition=(163.677, 
    104.149, -360.842), cameraUpVector=(0.700797, 0.50719, 0.501639), 
    cameraTarget=(108.401, 17.2196, 151.587), viewOffsetX=85.684, 
    viewOffsetY=-23.091)
session.viewports['Viewport: 1'].view.setValues(nearPlane=269.449, 
    farPlane=512.822, width=533.54, height=287.654, cameraPosition=(371.502, 
    79.6768, -191.381), cameraUpVector=(0.237942, 0.613605, 0.75291), 
    cameraTarget=(13.6976, 37.6976, 187.314), viewOffsetX=81.2635, 
    viewOffsetY=-21.8997)
session.viewports['Viewport: 1'].view.setValues(nearPlane=201.609, 
    farPlane=584.021, width=399.209, height=215.231, cameraPosition=(271.12, 
    -214.698, 31.8237), cameraUpVector=(0.512917, 0.70332, 0.492196), 
    cameraTarget=(-53.4528, 168.738, 176.133), viewOffsetX=60.8036, 
    viewOffsetY=-16.386)
session.viewports['Viewport: 1'].view.setValues(nearPlane=210.72, 
    farPlane=574.077, width=417.25, height=224.957, cameraPosition=(132.639, 
    430.578, -214.958), cameraUpVector=(0.352197, 0.244092, 0.903535), 
    cameraTarget=(150.029, 3.85379, 86.3727), viewOffsetX=63.5513, 
    viewOffsetY=-17.1265)
session.viewports['Viewport: 1'].view.setValues(nearPlane=268.579, 
    farPlane=518.048, width=531.818, height=286.726, cameraPosition=(-305.278, 
    357.756, -13.8117), cameraUpVector=(0.353868, 0.20766, 0.911951), 
    cameraTarget=(187.293, 187.258, 24.9272), viewOffsetX=81.0012, 
    viewOffsetY=-21.8291)
session.viewports['Viewport: 1'].view.setValues(nearPlane=199.858, 
    farPlane=586.051, width=395.743, height=213.362, cameraPosition=(-63.8175, 
    485.279, -130.642), cameraUpVector=(-0.282672, 0.0604407, 0.95731), 
    cameraTarget=(158.427, 98.2519, 141.414), viewOffsetX=60.2756, 
    viewOffsetY=-16.2437)
session.viewports['Viewport: 1'].view.setValues(nearPlane=289.854, 
    farPlane=496.139, width=573.946, height=309.439, cameraPosition=(419.528, 
    17.0442, -39.3912), cameraUpVector=(0.0371722, 0.778881, 0.626069), 
    cameraTarget=(-48.1811, 83.1945, 184.37), viewOffsetX=87.4178, 
    viewOffsetY=-23.5583)
session.viewports['Viewport: 1'].view.setValues(nearPlane=199.835, 
    farPlane=587.27, width=395.697, height=213.337, cameraPosition=(294.203, 
    -192.168, -29.6314), cameraUpVector=(0.558798, 0.764448, 0.321504), 
    cameraTarget=(-13.7957, 168.51, 190.015), viewOffsetX=60.2686, 
    viewOffsetY=-16.2418)
a = mdb.models['Model-1'].rootAssembly
s1 = a.instances['ke-1'].faces
side1Faces1 = s1.getSequenceFromMask(mask=('[#3 ]', ), )
region1=a.Surface(side1Faces=side1Faces1, name='m_Surf-7')
a = mdb.models['Model-1'].rootAssembly
s1 = a.instances['yao-1'].faces
side1Faces1 = s1.getSequenceFromMask(mask=('[#6 ]', ), )
region2=a.Surface(side1Faces=side1Faces1, name='s_Surf-7')
mdb.models['Model-1'].SurfaceToSurfaceContactStd(name='Int-3', 
    createStepName='Step-1', main=region1, secondary=region2, sliding=SMALL, 
    thickness=ON, interactionProperty='IntProp-1', adjustMethod=NONE, 
    initialClearance=OMIT, datumAxis=None, clearanceRegion=None)
#: 相互作用 "Int-3" 已创建.
session.viewports['Viewport: 1'].view.setValues(nearPlane=226.928, 
    farPlane=561.451, width=449.345, height=242.261, cameraPosition=(-69.6859, 
    361.182, -292.558), cameraUpVector=(0.632716, 0.542044, 0.553046), 
    cameraTarget=(188.559, 46.585, 35.3665), viewOffsetX=68.4397, 
    viewOffsetY=-18.4438)
i1 = mdb.models['Model-1'].rootAssembly.allInstances['ke-1']
i2 = mdb.models['Model-1'].rootAssembly.allInstances['yao-1']
leaf = dgm.LeafFromInstance(instances=(i1, i2, ))
session.viewports['Viewport: 1'].assemblyDisplay.displayGroup.replace(
    leaf=leaf)
session.viewports['Viewport: 1'].view.setValues(nearPlane=272.792, 
    farPlane=522.393, width=540.161, height=291.224, cameraPosition=(202.882, 
    -29.6532, -322.138), cameraUpVector=(0.513881, 0.829748, 0.217819), 
    cameraTarget=(118.896, 101.349, 176.842), viewOffsetX=82.2718, 
    viewOffsetY=-22.1714)
session.viewports['Viewport: 1'].view.setValues(nearPlane=266.643, 
    farPlane=530.89, width=527.986, height=284.66, cameraPosition=(-232.746, 
    109.488, -299.094), cameraUpVector=(0.757138, 0.585402, -0.289908), 
    cameraTarget=(179.79, 27.177, 11.1241), viewOffsetX=80.4175, 
    viewOffsetY=-21.6717)
session.viewports['Viewport: 1'].view.setValues(nearPlane=241.237, 
    farPlane=557.81, width=477.68, height=257.538, cameraPosition=(-324.186, 
    327.627, 97.2905), cameraUpVector=(0.70954, 0.700702, -0.07463), 
    cameraTarget=(99.238, 123.194, -130.998), viewOffsetX=72.7554, 
    viewOffsetY=-19.6068)
session.viewports['Viewport: 1'].view.setValues(nearPlane=205.536, 
    farPlane=591.084, width=406.989, height=219.425, cameraPosition=(-210.453, 
    405.052, -151.018), cameraUpVector=(0.93286, 0.24239, -0.266495), 
    cameraTarget=(104.093, -5.8592, -77.4719), viewOffsetX=61.9882, 
    viewOffsetY=-16.7052)
session.viewports['Viewport: 1'].view.setValues(nearPlane=230.83, 
    farPlane=566.5, width=457.074, height=246.428, cameraPosition=(-288.751, 
    371.834, -56.6516), cameraUpVector=(0.814708, 0.497483, -0.297929), 
    cameraTarget=(110.097, 37.984, -108.228), viewOffsetX=69.6166, 
    viewOffsetY=-18.761)
mdb.models['Model-1'].interactions['Int-3'].setValues(initialClearance=OMIT, 
    adjustMethod=NONE, sliding=FINITE, enforcement=SURFACE_TO_SURFACE, 
    thickness=ON, contactTracking=TWO_CONFIG, bondingSet=None)
mdb.saveAs(pathName='{{CAE_SAVE_PATH}}')

# t0执行成功标志
with open('t0_finished.flag', 'w') as f:
    f.write('success')

import sys
sys.exit()

