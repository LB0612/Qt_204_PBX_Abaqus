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


# =========================================================
# 2. 几何
# =========================================================
def revolve_part(name, lines, sheet=2000):
    sk = model.ConstrainedSketch(name='__profile__', sheetSize=sheet)
    sk.ConstructionLine(point1=(0, -sheet), point2=(0, sheet))
    sk.assignCenterline(sk.geometry[2])

    for line in lines:
        sk.Line(point1=line[0], point2=line[1])

    part = model.Part(
        name=name,
        dimensionality=THREE_D,
        type=DEFORMABLE_BODY
    )
    part.BaseSolidRevolve(sketch=sk, angle=90.0)
    return part


revolve_part('ke', [
    ((0, 0), (r + kt, 0)),
    ((r + kt, 0), (r + kt, h + kt)),
    ((r + kt, h + kt), (r, h + kt)),
    ((r, h + kt), (r, kt)),
    ((r, kt), (0, kt)),
    ((0, kt), (0, 0))
])

revolve_part('yao', [
    ((0, kt), (r, kt)),
    ((r, kt), (r, h + kt)),
    ((r, h + kt), (0, h + kt)),
    ((0, h + kt), (0, kt))
])


# =========================================================
# 3. 材料与截面
# =========================================================
model.Material(name='gang')
model.materials['gang'].Density(table=(({{MOLD_DENSITY}}, ), ))
model.materials['gang'].Elastic(table=((
    {{MOLD_ELASTIC_MODULUS}},
    {{MOLD_POISSON_RATIO}}
), ))
model.materials['gang'].Conductivity(
    table=(({{MOLD_THERMAL_CONDUCTIVITY}}, ), )
)
model.materials['gang'].SpecificHeat(
    table=(({{MOLD_SPECIFIC_HEAT}}, ), )
)

model.Material(name='pbx')
model.materials['pbx'].Density(table=(({{PBX_DENSITY}}, ), ))
model.materials['pbx'].Depvar(n=4)
model.materials['pbx'].UserDefinedField()
model.materials['pbx'].Elastic(
    dependencies=1,
    table=(
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
    )
)
model.materials['pbx'].Plastic(
    scaleStress=None,
    table=(({{PBX_YIELD_STRESS}}, 0.0), )
)
model.materials['pbx'].Expansion(
    table=(({{PBX_EXPANSION_COEFFICIENT}}, ), )
)
model.materials['pbx'].Conductivity(
    table=(({{PBX_THERMAL_CONDUCTIVITY}}, ), )
)
model.materials['pbx'].HeatGeneration()
model.materials['pbx'].SpecificHeat(
    table=(({{PBX_SPECIFIC_HEAT}}, ), )
)

model.HomogeneousSolidSection(
    name='ke',
    material='gang',
    thickness=None
)
model.HomogeneousSolidSection(
    name='yao',
    material='pbx',
    thickness=None
)

p = model.parts['yao']
cells = p.cells.getSequenceFromMask(mask=('[#1 ]', ), )
region = p.Set(cells=cells, name='Set-1')
p.SectionAssignment(
    region=region,
    sectionName='yao',
    offset=0.0,
    offsetType=MIDDLE_SURFACE,
    offsetField='',
    thicknessAssignment=FROM_SECTION
)

p = model.parts['ke']
cells = p.cells.getSequenceFromMask(mask=('[#1 ]', ), )
region = p.Set(cells=cells, name='Set-1')
p.SectionAssignment(
    region=region,
    sectionName='ke',
    offset=0.0,
    offsetType=MIDDLE_SURFACE,
    offsetField='',
    thicknessAssignment=FROM_SECTION
)


# =========================================================
# 4. 装配与分析步
# =========================================================
a = model.rootAssembly
a.DatumCsysByDefault(CARTESIAN)

a.Instance(name='ke-1', part=model.parts['ke'], dependent=OFF)
a.Instance(name='yao-1', part=model.parts['yao'], dependent=OFF)

model.CoupledTempDisplacementStep(
    name='Step-1',
    previous='Initial',
    timePeriod={{SIMULATION_TIME_LENGTH}},
    maxNumInc=1000000000,
    initialInc=0.0001,
    minInc=1e-05,
    maxInc=100000.0,
    deltmx=0.1
)

model.fieldOutputRequests['F-Output-1'].setValues(
    variables=('S', 'E', 'NT', 'SDV'),
    frequency=10
)


# =========================================================
# 5. 接触属性
# =========================================================
model.ContactProperty('IntProp-1')
model.interactionProperties['IntProp-1'].ThermalConductance(
    definition=TABULAR,
    clearanceDependency=ON,
    pressureDependency=OFF,
    temperatureDependencyC=OFF,
    massFlowRateDependencyC=OFF,
    dependenciesC=0,
    clearanceDepTable=((1000.0, 0.0), (0.0, 0.1))
)
model.interactionProperties['IntProp-1'].GeometricProperties(
    contactArea=1.0,
    padThickness=None
)


# =========================================================
# 6. 边界条件与初始温度
# =========================================================
faces = a.instances['yao-1'].faces.getSequenceFromMask(mask=('[#e ]', ), )
region = a.Set(faces=faces, name='Set-1')
model.DisplacementBC(
    name='BC-1',
    createStepName='Step-1',
    region=region,
    u1=0.0,
    u2=0.0,
    u3=0.0,
    ur1=UNSET,
    ur2=UNSET,
    ur3=UNSET,
    amplitude=UNSET,
    fixed=OFF,
    distributionType=UNIFORM,
    fieldName='',
    localCsys=None
)

faces = a.instances['ke-1'].faces.getSequenceFromMask(mask=('[#49 ]', ), )
region = a.Set(faces=faces, name='Set-2')
model.TemperatureBC(
    name='BC-2',
    createStepName='Step-1',
    region=region,
    fixed=OFF,
    distributionType=USER_DEFINED,
    fieldName='',
    magnitude={{AMBIENT_TEMPERATURE}},
    amplitude=UNSET
)

faces = a.instances['ke-1'].faces.getSequenceFromMask(mask=('[#22 ]', ), )
region = a.Set(faces=faces, name='Set-3')
model.EncastreBC(
    name='BC-3',
    createStepName='Step-1',
    region=region,
    localCsys=None
)

cells1 = a.instances['ke-1'].cells.getSequenceFromMask(mask=('[#1 ]', ), )
faces1 = a.instances['ke-1'].faces.getSequenceFromMask(mask=('[#7f ]', ), )
edges1 = a.instances['ke-1'].edges.getSequenceFromMask(mask=('[#1ffff ]', ), )
verts1 = a.instances['ke-1'].vertices.getSequenceFromMask(mask=('[#fff ]', ), )

cells2 = a.instances['yao-1'].cells.getSequenceFromMask(mask=('[#1 ]', ), )
faces2 = a.instances['yao-1'].faces.getSequenceFromMask(mask=('[#1f ]', ), )
edges2 = a.instances['yao-1'].edges.getSequenceFromMask(mask=('[#1ff ]', ), )
verts2 = a.instances['yao-1'].vertices.getSequenceFromMask(mask=('[#3f ]', ), )

region = a.Set(
    vertices=verts1 + verts2,
    edges=edges1 + edges2,
    faces=faces1 + faces2,
    cells=cells1 + cells2,
    name='Set-4'
)
model.Temperature(
    name='Predefined Field-1',
    createStepName='Initial',
    region=region,
    distributionType=UNIFORM,
    crossSectionDistribution=CONSTANT_THROUGH_THICKNESS,
    magnitudes=({{AMBIENT_TEMPERATURE}}, )
)


# =========================================================
# 7. 网格
# =========================================================
ke_instance = a.instances['ke-1']
ke_regions = (ke_instance, )

a.seedPartInstance(
    regions=ke_regions,
    size=6.0,
    deviationFactor=0.1,
    minSizeFactor=0.1
)

picked_regions = ke_instance.cells.getSequenceFromMask(mask=('[#1 ]', ), )
a.setMeshControls(
    regions=picked_regions,
    elemShape=HEX_DOMINATED,
    technique=SWEEP,
    algorithm=ADVANCING_FRONT
)
a.generateMesh(regions=ke_regions)

elemType1 = mesh.ElemType(
    elemCode=C3D8T,
    elemLibrary=STANDARD,
    secondOrderAccuracy=OFF,
    distortionControl=DEFAULT
)
elemType2 = mesh.ElemType(elemCode=C3D6T, elemLibrary=STANDARD)
elemType3 = mesh.ElemType(elemCode=C3D4T, elemLibrary=STANDARD)

cells1 = ke_instance.cells.getSequenceFromMask(mask=('[#1 ]', ), )
a.setElementType(
    regions=(cells1, ),
    elemTypes=(elemType1, elemType2, elemType3)
)

yao_instance = a.instances['yao-1']
yao_regions = (yao_instance, )

a.seedPartInstance(
    regions=yao_regions,
    size=4.0,
    deviationFactor=0.1,
    minSizeFactor=0.1
)

picked_regions = yao_instance.cells.getSequenceFromMask(mask=('[#1 ]', ), )
a.setMeshControls(
    regions=picked_regions,
    elemShape=HEX_DOMINATED
)
a.generateMesh(regions=yao_regions)

elemType1 = mesh.ElemType(
    elemCode=C3D8T,
    elemLibrary=STANDARD,
    secondOrderAccuracy=OFF,
    distortionControl=DEFAULT
)
elemType2 = mesh.ElemType(elemCode=C3D6T, elemLibrary=STANDARD)
elemType3 = mesh.ElemType(elemCode=C3D4T, elemLibrary=STANDARD)

cells1 = yao_instance.cells.getSequenceFromMask(mask=('[#1 ]', ), )
a.setElementType(
    regions=(cells1, ),
    elemTypes=(elemType1, elemType2, elemType3)
)


# =========================================================
# 8. 最终相互作用
# =========================================================
side1_faces = a.instances['ke-1'].faces.getSequenceFromMask(mask=('[#1c ]', ), )
region = a.Surface(side1Faces=side1_faces, name='Surf-5')
model.FilmCondition(
    name='Int-1',
    createStepName='Step-1',
    surface=region,
    definition=USER_SUB,
    filmCoeff=10.0,
    sinkTemperature=1.0,
    sinkDistributionType=UNIFORM,
    sinkFieldName=''
)

side1_faces = a.instances['yao-1'].faces.getSequenceFromMask(mask=('[#1 ]', ), )
region = a.Surface(side1Faces=side1_faces, name='Surf-6')
model.FilmCondition(
    name='Int-2',
    createStepName='Step-1',
    surface=region,
    definition=USER_SUB,
    filmCoeff=8.0,
    sinkTemperature=1.0,
    sinkDistributionType=UNIFORM,
    sinkFieldName=''
)

side1_faces = a.instances['ke-1'].faces.getSequenceFromMask(mask=('[#3 ]', ), )
region1 = a.Surface(side1Faces=side1_faces, name='m_Surf-7')

side1_faces = a.instances['yao-1'].faces.getSequenceFromMask(mask=('[#6 ]', ), )
region2 = a.Surface(side1Faces=side1_faces, name='s_Surf-7')

model.SurfaceToSurfaceContactStd(
    name='Int-3',
    createStepName='Step-1',
    main=region1,
    secondary=region2,
    sliding=SMALL,
    thickness=ON,
    interactionProperty='IntProp-1',
    adjustMethod=NONE,
    initialClearance=OMIT,
    datumAxis=None,
    clearanceRegion=None
)

model.interactions['Int-3'].setValues(
    initialClearance=OMIT,
    adjustMethod=NONE,
    sliding=FINITE,
    enforcement=SURFACE_TO_SURFACE,
    thickness=ON,
    contactTracking=TWO_CONFIG,
    bondingSet=None
)


# =========================================================
# 9. 保存
# =========================================================
mdb.saveAs(pathName='{{CAE_SAVE_PATH}}')

# t0执行成功标志
with open('t0_finished.flag', 'w') as f:
    f.write('success')

import sys
sys.exit()
