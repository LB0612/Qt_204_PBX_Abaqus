# -*- coding: utf-8 -*-

from abaqus import *
from abaqusConstants import *
from caeModules import *
import displayGroupOdbToolset as dgo
import json
import os
import shutil
import subprocess
import sys

# t2: ODB post-processing only (PNG export + FFmpeg MP4).

VIDEO_FPS = 5
STEP_NAME = 'Step-1'
PNG_SIGNATURE = b'\x89PNG\r\n\x1a\n'
MIN_PNG_SIZE = 64

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
    _atomic_write_text(path, json.dumps(payload, indent=2, sort_keys=True))


def _env_path(name):
    value = os.environ.get(name, '').strip()
    if not value:
        raise RuntimeError('Missing environment variable: %s' % name)
    if not os.path.isfile(value):
        raise RuntimeError('File not found for %s: %s' % (name, value))
    return value


def _ffmpeg_exe():
    return _env_path('PBX_FFMPEG_EXE')


def _ffprobe_exe():
    return _env_path('PBX_FFPROBE_EXE')


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
        cure_output_base + '.mp4',
        temp_output_base + '.mp4',
        stress_output_base + '.mp4',
        cure_output_base + '.tmp.mp4',
        temp_output_base + '.tmp.mp4',
        stress_output_base + '.tmp.mp4',
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

    if os.path.isdir(results_dir):
        for name in os.listdir(results_dir):
            if name.lower().endswith('.tmp.mp4'):
                extra_path = os.path.join(results_dir, name)
                if os.path.isfile(extra_path):
                    os.remove(extra_path)
                    print('[POST] Removed file: %s' % extra_path)


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

    return b'IEND' in tail


def _ensure_frame_dir(frame_dir):
    if not os.path.isdir(frame_dir):
        os.makedirs(frame_dir)


def _frame_png_path(frame_dir, prefix, index):
    return os.path.join(frame_dir, '%s_%08d.png' % (prefix, index))


def _verify_png_sequence(frame_dir, prefix, expected_count):
    for index in range(expected_count):
        path = _frame_png_path(frame_dir, prefix, index)
        if not _is_valid_png(path):
            raise RuntimeError(
                'Missing or invalid PNG frame %d: %s'
                % (index, path)
            )


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
        raise RuntimeError('ODB step not found: %s' % step_name)

    frames = odb.steps[step_name].frames
    if len(frames) == 0:
        raise RuntimeError('%s contains no frames.' % step_name)

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
        png_path = _frame_png_path(frame_dir, frame_prefix, index)
        if _is_valid_png(png_path):
            print(
                '[POST] Skip existing valid PNG: %s'
                % os.path.basename(png_path)
            )
            continue

        vp.odbDisplay.setFrame(step=0, frame=index)
        file_base = os.path.join(
            frame_dir,
            '%s_%08d' % (frame_prefix, index)
        )

        session.printToFile(
            fileName=file_base,
            format=PNG,
            canvasObjects=(vp,)
        )

        if not _is_valid_png(png_path):
            raise IOError('PNG export failed: %s' % png_path)

        print(
            '[POST] %s: exported frame %d / %d'
            % (variable_label, index, len(frames) - 1)
        )

    return len(frames)


def _count_video_frames(video_path):
    ffprobe = _ffprobe_exe()
    result = subprocess.run(
        [
            ffprobe,
            '-v', 'error',
            '-select_streams', 'v:0',
            '-count_frames',
            '-show_entries', 'stream=nb_read_frames',
            '-of', 'default=nokey=1:noprint_wrappers=1',
            video_path,
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    if result.returncode != 0:
        raise RuntimeError(
            'FFprobe failed for %s: %s'
            % (video_path, result.stderr.decode('utf-8', 'ignore'))
        )

    text = result.stdout.decode('utf-8', 'ignore').strip()
    if not text.isdigit():
        raise RuntimeError(
            'FFprobe returned invalid frame count for %s: %s'
            % (video_path, text)
        )

    return int(text)


def _video_is_valid(video_path, expected_frames):
    if not os.path.isfile(video_path):
        return False

    try:
        return _count_video_frames(video_path) == expected_frames
    except Exception as error:
        print('[POST] Existing video invalid: %s (%s)' % (video_path, error))
        return False


def _generate_mp4(frame_dir, frame_prefix, output_mp4, expected_frames):
    if _video_is_valid(output_mp4, expected_frames):
        print('[POST] Skip existing valid MP4: %s' % output_mp4)
        return expected_frames

    tmp_mp4 = output_mp4 + '.tmp.mp4'
    if os.path.isfile(tmp_mp4):
        os.remove(tmp_mp4)

    input_pattern = os.path.join(frame_dir, frame_prefix + '_%08d.png')
    ffmpeg = _ffmpeg_exe()

    result = subprocess.run(
        [
            ffmpeg,
            '-y',
            '-hide_banner',
            '-loglevel', 'error',
            '-framerate', str(VIDEO_FPS),
            '-start_number', '0',
            '-i', input_pattern,
            '-vf', 'pad=ceil(iw/2)*2:ceil(ih/2)*2',
            '-frames:v', str(expected_frames),
            '-c:v', 'libx264',
            '-pix_fmt', 'yuv420p',
            tmp_mp4,
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    if result.returncode != 0:
        if os.path.isfile(tmp_mp4):
            os.remove(tmp_mp4)
        raise RuntimeError(
            'FFmpeg failed for %s: %s'
            % (output_mp4, result.stderr.decode('utf-8', 'ignore'))
        )

    actual_frames = _count_video_frames(tmp_mp4)
    if actual_frames != expected_frames:
        if os.path.isfile(tmp_mp4):
            os.remove(tmp_mp4)
        raise RuntimeError(
            'MP4 frame mismatch for %s: expected %d, got %d'
            % (output_mp4, expected_frames, actual_frames)
        )

    os.replace(tmp_mp4, output_mp4)
    print('[POST] MP4 generated: %s (%d frames)' % (output_mp4, actual_frames))
    return actual_frames


def _setup_viewport(odb):
    vp = session.viewports['Viewport: 1']
    vp.setValues(displayedObject=odb)
    vp.makeCurrent()
    vp.maximize()

    leaf = dgo.LeafFromOdbElementMaterials(
        elementMaterials=('PBX', )
    )
    vp.odbDisplay.displayGroup.replace(leaf=leaf)

    vp.odbDisplay.display.setValues(
        plotState=(CONTOURS_ON_DEF, )
    )

    vp.view.setValues(
        nearPlane=397.358,
        farPlane=608.793,
        width=379.342,
        height=190.64,
        cameraPosition=(-335.563, 163.981, -322.396),
        cameraUpVector=(0.298021, 0.907506, 0.296002),
        cameraTarget=(23.2562, 117.183, 23.6633)
    )

    return vp


os.chdir(work_dir)

if _t2_reset_requested():
    print('[POST] PBX_T2_RESET=1, clearing previous post-process outputs.')
    _reset_postprocess_outputs()
elif os.path.isfile(t2_flag_path):
    os.remove(t2_flag_path)

if not os.path.isfile(odb_path):
    raise IOError('ODB not found: %s' % odb_path)

print('=========================================')
print('t2 post-processing start')
print('ODB: %s' % odb_path)
print('=========================================')

_ffmpeg_exe()
_ffprobe_exe()

odb = session.openOdb(name=odb_path)
vp = _setup_viewport(odb)

if STEP_NAME not in odb.steps.keys():
    raise RuntimeError('ODB step not found: %s' % STEP_NAME)

expected_frames = len(odb.steps[STEP_NAME].frames)
if expected_frames == 0:
    raise RuntimeError('%s contains no frames.' % STEP_NAME)

print('[POST] ODB frames = %d' % expected_frames)

# Phase A: export all PNG sequences before any video work.
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

if not (cure_count == temp_count == stress_count == expected_frames):
    raise RuntimeError(
        'PNG export count mismatch: ODB=%d Cure=%d Temp=%d Stress=%d'
        % (expected_frames, cure_count, temp_count, stress_count)
    )

_verify_png_sequence(cure_output_base + '_frames', 'Cure_SDV1_frame', expected_frames)
_verify_png_sequence(temp_output_base + '_frames', 'NT11_frame', expected_frames)
_verify_png_sequence(stress_output_base + '_frames', 'Stress_Mises_frame', expected_frames)

print('[POST] All ODB frames exported successfully.')
print('[POST] ODB frames = %d' % expected_frames)
print('[POST] Cure = %d' % cure_count)
print('[POST] Temperature = %d' % temp_count)
print('[POST] Stress = %d' % stress_count)

# Phase B: generate MP4 videos only after all PNG sets are complete.
cure_video_frames = _generate_mp4(
    cure_output_base + '_frames',
    'Cure_SDV1_frame',
    cure_output_base + '.mp4',
    expected_frames
)
temp_video_frames = _generate_mp4(
    temp_output_base + '_frames',
    'NT11_frame',
    temp_output_base + '.mp4',
    expected_frames
)
stress_video_frames = _generate_mp4(
    stress_output_base + '_frames',
    'Stress_Mises_frame',
    stress_output_base + '.mp4',
    expected_frames
)

post_sha = _post_sha256()
manifest = {
    'version': 1,
    'postSha256': post_sha,
    'step': STEP_NAME,
    'odbFrames': expected_frames,
    'curePngFrames': cure_count,
    'temperaturePngFrames': temp_count,
    'stressPngFrames': stress_count,
    'cureVideoFrames': cure_video_frames,
    'temperatureVideoFrames': temp_video_frames,
    'stressVideoFrames': stress_video_frames,
    'videoFps': VIDEO_FPS,
}

_atomic_write_json(manifest_path, manifest)
_atomic_write_text(t2_flag_path, 'success')

print('=========================================')
print('t2 post-processing finished successfully.')
print('Manifest: %s' % manifest_path)
print('Cure MP4: %s.mp4' % cure_output_base)
print('Temperature MP4: %s.mp4' % temp_output_base)
print('Stress MP4: %s.mp4' % stress_output_base)
print('=========================================')

sys.exit(0)
