# -*- coding: utf-8 -*-

from abaqus import *
from abaqusConstants import *
from caeModules import *
import displayGroupOdbToolset as dgo
import json
import math
import os
import shutil
import sys

# t2: ODB post-processing only (PNG export).

PLAYBACK_FPS = 5
STEP_NAME = 'Step-1'

PNG_SIGNATURE = b'\x89PNG\r\n\x1a\n'
MIN_PNG_SIZE = 64

EXPORT_IMAGE_SIZE = (2456, 1238)
EXPORT_ASPECT = (
    float(EXPORT_IMAGE_SIZE[0])
    / float(EXPORT_IMAGE_SIZE[1])
)
EXPORT_VIEWPORT_WIDTH_MM = 320.0

RPY_CAMERA_DIRECTION = (
    -0.707443360774569,
     0.171467008152341,
    -0.685655129355325,
)
RPY_CAMERA_UP = (
    0.338782,
    0.871506,
    0.354548,
)

RPY_VIEW_SCALE = 1.22631
RPY_OFFSET_X_RATIO = 0.0208083
RPY_OFFSET_Y_RATIO = -0.0894107

work_dir = '{{ABAQUS_WORK_DIR}}'
job_name = '{{JOB_NAME}}'
stress_output_base = '{{RESULT_STRESS_PATH}}'
temp_output_base = '{{RESULT_TEMP_PATH}}'
cure_output_base = '{{RESULT_CURE_PATH}}'

results_dir = os.path.dirname(stress_output_base)
t2_flag_path = os.path.join(work_dir, 't2_finished.flag')
manifest_path = os.path.join(results_dir, 'postprocess_manifest.json')
odb_path = os.path.join(work_dir, job_name + '.odb')


def _atomic_write_text(path, content):
    tmp_path = path + '.tmp'
    with open(tmp_path, 'w') as handle:
        handle.write(content)
        handle.flush()
        os.fsync(handle.fileno())
    os.replace(tmp_path, path)


def _atomic_write_json(path, payload):
    _atomic_write_text(
        path,
        json.dumps(payload, indent=2, sort_keys=True)
    )


def _post_sha256():
    return os.environ.get('PBX_POST_SHA256', '').strip()


def _t2_reset_requested():
    return os.environ.get('PBX_T2_RESET', '0').strip() == '1'


def _reset_postprocess_outputs():
    frame_dirs = [
        cure_output_base + '_frames',
        temp_output_base + '_frames',
        stress_output_base + '_frames',
    ]
    files_to_remove = [
        manifest_path,
        t2_flag_path,
    ]

    for frame_dir in frame_dirs:
        if os.path.isdir(frame_dir):
            shutil.rmtree(frame_dir)
            print('[POST] Removed frame directory: %s' % frame_dir)

    for path in files_to_remove:
        if os.path.isfile(path):
            os.remove(path)
            print('[POST] Removed file: %s' % path)


def _is_valid_png(path):
    if not os.path.isfile(path):
        return False

    size = os.path.getsize(path)
    if size < MIN_PNG_SIZE:
        return False

    with open(path, 'rb') as handle:
        header = handle.read(8)
        if header != PNG_SIGNATURE:
            return False

        handle.seek(-12, os.SEEK_END)
        tail = handle.read(12)

    return tail[4:8] == b'IEND'


def _ensure_frame_dir(frame_dir):
    if not os.path.isdir(frame_dir):
        os.makedirs(frame_dir)


def _frame_png_path(frame_dir, prefix, index):
    return os.path.join(
        frame_dir,
        '%s_%08d.png' % (prefix, index)
    )


def _verify_png_sequence(frame_dir, prefix, expected_count):
    for index in range(expected_count):
        path = _frame_png_path(
            frame_dir,
            prefix,
            index
        )
        if not _is_valid_png(path):
            raise RuntimeError(
                'Missing or invalid PNG frame %d: %s'
                % (index, path)
            )


def _vector_length(vector):
    return math.sqrt(
        vector[0] * vector[0]
        + vector[1] * vector[1]
        + vector[2] * vector[2]
    )


def _prepare_export_viewport(vp):
    vp.makeCurrent()

    try:
        vp.restore()
        vp.setValues(
            origin=(0.0, 0.0),
            width=EXPORT_VIEWPORT_WIDTH_MM,
            height=EXPORT_VIEWPORT_WIDTH_MM / EXPORT_ASPECT
        )
    except Exception as exc:
        print(
            '[POST] Custom viewport unavailable, '
            'falling back to maximize: %s' % exc
        )
        vp.maximize()

    session.printOptions.setValues(
        rendition=COLOR,
        vpDecorations=OFF,
        vpBackground=OFF,
        reduceColors=False
    )

    session.pngOptions.setValues(
        imageSize=EXPORT_IMAGE_SIZE
    )


def _apply_rpy_adaptive_view(vp, expected_frames):
    fit_frame = max(0, expected_frames - 1)
    vp.odbDisplay.setFrame(
        step=0,
        frame=fit_frame
    )

    vp.view.fitView()

    target = tuple(vp.view.cameraTarget)
    position = tuple(vp.view.cameraPosition)

    current_vector = (
        position[0] - target[0],
        position[1] - target[1],
        position[2] - target[2],
    )
    distance = _vector_length(current_vector)

    if distance <= 0.0:
        raise RuntimeError(
            'Abaqus produced an invalid camera distance.'
        )

    adaptive_position = (
        target[0] + RPY_CAMERA_DIRECTION[0] * distance,
        target[1] + RPY_CAMERA_DIRECTION[1] * distance,
        target[2] + RPY_CAMERA_DIRECTION[2] * distance,
    )

    vp.view.setValues(
        cameraPosition=adaptive_position,
        cameraUpVector=RPY_CAMERA_UP,
        cameraTarget=target
    )

    vp.view.fitView()

    fitted_width = float(vp.view.width)
    fitted_height = float(vp.view.height)

    if fitted_width <= 0.0 or fitted_height <= 0.0:
        raise RuntimeError(
            'Automatic viewport fitting produced invalid dimensions.'
        )

    final_width = fitted_width * RPY_VIEW_SCALE
    final_height = fitted_height * RPY_VIEW_SCALE

    vp.view.setValues(
        width=final_width,
        height=final_height,
        viewOffsetX=final_width * RPY_OFFSET_X_RATIO,
        viewOffsetY=final_height * RPY_OFFSET_Y_RATIO
    )

    print(
        '[POST] RPY-style adaptive view prepared: '
        'frame=%d fit=(%.6g, %.6g) final=(%.6g, %.6g) '
        'offset=(%.6g, %.6g)'
        % (
            fit_frame,
            fitted_width,
            fitted_height,
            final_width,
            final_height,
            final_width * RPY_OFFSET_X_RATIO,
            final_height * RPY_OFFSET_Y_RATIO
        )
    )

    vp.odbDisplay.setFrame(
        step=0,
        frame=0
    )


def _setup_viewport(odb, expected_frames):
    vp = session.viewports['Viewport: 1']

    vp.setValues(
        displayedObject=odb
    )
    vp.makeCurrent()

    _prepare_export_viewport(vp)

    leaf = dgo.LeafFromOdbElementMaterials(
        elementMaterials=('PBX', )
    )
    vp.odbDisplay.displayGroup.replace(
        leaf=leaf
    )

    vp.odbDisplay.display.setValues(
        plotState=(CONTOURS_ON_DEF, )
    )

    _apply_rpy_adaptive_view(
        vp,
        expected_frames
    )

    return vp


def _export_png_frames(
    vp,
    odb,
    step_name,
    variable_label,
    output_position,
    output_base,
    frame_prefix,
    refinement=None
):
    if step_name not in odb.steps.keys():
        raise RuntimeError(
            'ODB step not found: %s'
            % step_name
        )

    frames = odb.steps[step_name].frames

    if len(frames) == 0:
        raise RuntimeError(
            '%s contains no frames.'
            % step_name
        )

    frame_dir = output_base + '_frames'
    parent_dir = os.path.dirname(output_base)

    if parent_dir and not os.path.isdir(parent_dir):
        os.makedirs(parent_dir)

    _ensure_frame_dir(frame_dir)

    if refinement is None:
        vp.odbDisplay.setPrimaryVariable(
            variableLabel=variable_label,
            outputPosition=output_position
        )
    else:
        vp.odbDisplay.setPrimaryVariable(
            variableLabel=variable_label,
            outputPosition=output_position,
            refinement=refinement
        )

    for index in range(len(frames)):
        png_path = _frame_png_path(
            frame_dir,
            frame_prefix,
            index
        )

        if _is_valid_png(png_path):
            print(
                '[POST] Skip existing valid PNG: %s'
                % os.path.basename(png_path)
            )
            continue

        vp.odbDisplay.setFrame(
            step=0,
            frame=index
        )

        file_base = os.path.join(
            frame_dir,
            '%s_%08d'
            % (frame_prefix, index)
        )

        session.printToFile(
            fileName=file_base,
            format=PNG,
            canvasObjects=(vp,)
        )

        if not _is_valid_png(png_path):
            raise IOError(
                'PNG export failed: %s'
                % png_path
            )

        print(
            '[POST] %s: exported frame %d / %d'
            % (
                variable_label,
                index,
                len(frames) - 1
            )
        )

    return len(frames)


os.chdir(work_dir)

if _t2_reset_requested():
    print(
        '[POST] PBX_T2_RESET=1, '
        'clearing previous post-process outputs.'
    )
    _reset_postprocess_outputs()
elif os.path.isfile(t2_flag_path):
    os.remove(t2_flag_path)

if not os.path.isfile(odb_path):
    raise IOError(
        'ODB not found: %s'
        % odb_path
    )

print('=========================================')
print('t2 post-processing start')
print('ODB: %s' % odb_path)
print('=========================================')

odb = session.openOdb(
    name=odb_path
)

if STEP_NAME not in odb.steps.keys():
    raise RuntimeError(
        'ODB step not found: %s'
        % STEP_NAME
    )

expected_frames = len(
    odb.steps[STEP_NAME].frames
)

if expected_frames == 0:
    raise RuntimeError(
        '%s contains no frames.'
        % STEP_NAME
    )

vp = _setup_viewport(
    odb,
    expected_frames
)

print(
    '[POST] ODB frames = %d'
    % expected_frames
)

odb_frames = odb.steps[STEP_NAME].frames
frame_times = [
    float(odb_frames[i].frameValue)
    for i in range(len(odb_frames))
]

cure_count = _export_png_frames(
    vp=vp,
    odb=odb,
    step_name=STEP_NAME,
    variable_label='SDV1',
    output_position=INTEGRATION_POINT,
    output_base=cure_output_base,
    frame_prefix='Cure_SDV1_frame'
)

temp_count = _export_png_frames(
    vp=vp,
    odb=odb,
    step_name=STEP_NAME,
    variable_label='NT11',
    output_position=NODAL,
    output_base=temp_output_base,
    frame_prefix='NT11_frame'
)

stress_count = _export_png_frames(
    vp=vp,
    odb=odb,
    step_name=STEP_NAME,
    variable_label='S',
    output_position=INTEGRATION_POINT,
    refinement=(INVARIANT, 'Mises'),
    output_base=stress_output_base,
    frame_prefix='Stress_Mises_frame'
)

if not (
    cure_count == temp_count
    == stress_count
    == expected_frames
):
    raise RuntimeError(
        'PNG export count mismatch: '
        'ODB=%d Cure=%d Temp=%d Stress=%d'
        % (
            expected_frames,
            cure_count,
            temp_count,
            stress_count
        )
    )

_verify_png_sequence(
    cure_output_base + '_frames',
    'Cure_SDV1_frame',
    expected_frames
)

_verify_png_sequence(
    temp_output_base + '_frames',
    'NT11_frame',
    expected_frames
)

_verify_png_sequence(
    stress_output_base + '_frames',
    'Stress_Mises_frame',
    expected_frames
)

print(
    '[POST] All ODB frames exported successfully.'
)
print(
    '[POST] ODB frames = %d'
    % expected_frames
)
print(
    '[POST] Cure = %d'
    % cure_count
)
print(
    '[POST] Temperature = %d'
    % temp_count
)
print(
    '[POST] Stress = %d'
    % stress_count
)

post_sha = _post_sha256()

manifest = {
    'version': {{POSTPROCESS_MANIFEST_VERSION}},
    'postSha256': post_sha,
    'step': STEP_NAME,
    'odbFrames': expected_frames,
    'curePngFrames': cure_count,
    'temperaturePngFrames': temp_count,
    'stressPngFrames': stress_count,
    'playbackFps': PLAYBACK_FPS,
    'frameTimes': frame_times,
}

_atomic_write_json(
    manifest_path,
    manifest
)

_atomic_write_text(
    t2_flag_path,
    'success'
)

print('=========================================')
print(
    't2 post-processing finished successfully.'
)
print(
    'Manifest: %s'
    % manifest_path
)
print(
    'Cure PNG: %s_frames'
    % cure_output_base
)
print(
    'Temperature PNG: %s_frames'
    % temp_output_base
)
print(
    'Stress PNG: %s_frames'
    % stress_output_base
)
print('=========================================')

sys.exit(0)
