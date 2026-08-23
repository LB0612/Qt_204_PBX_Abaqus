# -*- coding: utf-8 -*-

from abaqus import *
from abaqusConstants import *
from caeModules import *
import displayGroupOdbToolset as dgo
import os
import sys
import struct
import zlib

# =========================================================
# t1: submit Abaqus Job with 335K.for, then export real ODB
# saved frames for cure (SDV1), temperature (NT11), and
# stress (S, Mises) to PNG sequences and uncompressed AVI.
#
# The viewport/filter/camera settings follow the user's
# verified Abaqus 2025 replay (.rpy) workflow.
# =========================================================

VIDEO_FPS = 5
STEP_NAME = 'Step-1'

# Existing Qt placeholders. Keep these names unchanged so
# AbaqusFileGenerator.cpp can replace them directly.
work_dir = '{{ABAQUS_WORK_DIR}}'
cae_path = '{{CAE_FILE_PATH}}'
user_subroutine_path = '{{USER_SUBROUTINE_PATH}}'
job_name = '{{JOB_NAME}}'
stress_output_base = '{{RESULT_STRESS_PATH}}'
temp_output_base = '{{RESULT_TEMP_PATH}}'
cure_output_base = '{{RESULT_CURE_PATH}}'

os.chdir(work_dir)

# Remove only the completion marker from a previous t1 run.
# Qt also removes it before launch, but this keeps t1 safe when
# it is run manually.
t1_flag_path = os.path.join(work_dir, 't1_finished.flag')
if os.path.isfile(t1_flag_path):
    os.remove(t1_flag_path)


# =========================================================
# PNG reader: pure Python, no third-party packages
# =========================================================

def _paeth(a, b, c):
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def _read_png_rgb(path):
    with open(path, 'rb') as f:
        data = f.read()

    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise RuntimeError('Invalid PNG file: %s' % path)

    pos = 8
    width = None
    height = None
    bit_depth = None
    color_type = None
    interlace = None
    idat = []
    palette = None
    transparency = None

    while pos < len(data):
        if pos + 8 > len(data):
            break

        length = struct.unpack('>I', data[pos:pos + 4])[0]
        chunk_type = data[pos + 4:pos + 8]
        chunk_data = data[pos + 8:pos + 8 + length]
        pos += 12 + length

        if chunk_type == b'IHDR':
            (
                width,
                height,
                bit_depth,
                color_type,
                compression,
                filter_method,
                interlace
            ) = struct.unpack('>IIBBBBB', chunk_data)

        elif chunk_type == b'IDAT':
            idat.append(chunk_data)

        elif chunk_type == b'PLTE':
            palette = []
            for i in range(0, len(chunk_data), 3):
                palette.append(
                    (
                        chunk_data[i],
                        chunk_data[i + 1],
                        chunk_data[i + 2]
                    )
                )

        elif chunk_type == b'tRNS':
            transparency = chunk_data

        elif chunk_type == b'IEND':
            break

    if width is None or height is None:
        raise RuntimeError('PNG missing IHDR: %s' % path)

    if bit_depth != 8:
        raise RuntimeError(
            'Only 8-bit PNG is supported, bit depth = %s' % bit_depth
        )

    if interlace != 0:
        raise RuntimeError('Interlaced PNG is not supported.')

    bytes_per_pixel = {
        0: 1,
        2: 3,
        3: 1,
        4: 2,
        6: 4,
    }.get(color_type)

    if bytes_per_pixel is None:
        raise RuntimeError(
            'Unsupported PNG color type = %s' % color_type
        )

    raw = zlib.decompress(b''.join(idat))
    stride = width * bytes_per_pixel
    rows = []

    offset = 0
    previous = bytearray(stride)

    for y in range(height):
        filter_type = raw[offset]
        offset += 1

        scan = bytearray(raw[offset:offset + stride])
        offset += stride

        reconstructed = bytearray(stride)

        for x in range(stride):
            left = (
                reconstructed[x - bytes_per_pixel]
                if x >= bytes_per_pixel
                else 0
            )
            up = previous[x]
            up_left = (
                previous[x - bytes_per_pixel]
                if x >= bytes_per_pixel
                else 0
            )

            value = scan[x]

            if filter_type == 0:
                result = value
            elif filter_type == 1:
                result = (value + left) & 255
            elif filter_type == 2:
                result = (value + up) & 255
            elif filter_type == 3:
                result = (value + ((left + up) // 2)) & 255
            elif filter_type == 4:
                result = (value + _paeth(left, up, up_left)) & 255
            else:
                raise RuntimeError(
                    'Unsupported PNG filter type = %s' % filter_type
                )

            reconstructed[x] = result

        rows.append(bytes(reconstructed))
        previous = reconstructed

    rgb_rows = []

    for row in rows:
        out = bytearray(width * 3)

        if color_type == 2:
            out[:] = row

        elif color_type == 6:
            for x in range(width):
                r = row[x * 4]
                g = row[x * 4 + 1]
                b = row[x * 4 + 2]
                a = row[x * 4 + 3]

                out[x * 3] = (
                    r * a + 255 * (255 - a)
                ) // 255
                out[x * 3 + 1] = (
                    g * a + 255 * (255 - a)
                ) // 255
                out[x * 3 + 2] = (
                    b * a + 255 * (255 - a)
                ) // 255

        elif color_type == 0:
            for x in range(width):
                v = row[x]
                out[x * 3:x * 3 + 3] = bytes((v, v, v))

        elif color_type == 4:
            for x in range(width):
                v = row[x * 2]
                a = row[x * 2 + 1]
                value = (
                    v * a + 255 * (255 - a)
                ) // 255
                out[x * 3:x * 3 + 3] = bytes(
                    (value, value, value)
                )

        elif color_type == 3:
            if palette is None:
                raise RuntimeError('PNG palette is missing.')

            for x in range(width):
                index = row[x]
                r, g, b = palette[index]
                a = 255

                if (
                    transparency is not None
                    and index < len(transparency)
                ):
                    a = transparency[index]

                out[x * 3] = (
                    r * a + 255 * (255 - a)
                ) // 255
                out[x * 3 + 1] = (
                    g * a + 255 * (255 - a)
                ) // 255
                out[x * 3 + 2] = (
                    b * a + 255 * (255 - a)
                ) // 255

        rgb_rows.append(bytes(out))

    return width, height, rgb_rows


# =========================================================
# AVI writer: uncompressed 24-bit BGR
# =========================================================

def _make_avi_frame(png_path):
    width, height, rgb_rows = _read_png_rgb(png_path)

    raw_row_size = width * 3
    row_padding = (-raw_row_size) % 4
    padding = b'\x00' * row_padding

    frame = bytearray()

    # Windows DIB is bottom-up.
    for row in reversed(rgb_rows):
        bgr = bytearray(raw_row_size)

        for x in range(width):
            r = row[x * 3]
            g = row[x * 3 + 1]
            b = row[x * 3 + 2]

            bgr[x * 3] = b
            bgr[x * 3 + 1] = g
            bgr[x * 3 + 2] = r

        frame.extend(bgr)
        frame.extend(padding)

    return width, height, bytes(frame)


def _riff_chunk(fourcc, payload):
    result = (
        fourcc
        + struct.pack('<I', len(payload))
        + payload
    )

    if len(payload) % 2:
        result += b'\x00'

    return result


def _riff_list(list_type, payload):
    body = list_type + payload
    return (
        b'LIST'
        + struct.pack('<I', len(body))
        + body
    )


def _write_uncompressed_avi(png_paths, avi_path, fps):
    if not png_paths:
        raise RuntimeError('No PNG frames; AVI cannot be created.')

    width, height, first_frame = _make_avi_frame(png_paths[0])

    frames = [first_frame]
    frame_size = len(first_frame)

    for path in png_paths[1:]:
        w, h, frame_data = _make_avi_frame(path)

        if w != width or h != height:
            raise RuntimeError('All PNG frames must have the same size.')

        frames.append(frame_data)

    total_frames = len(frames)
    microseconds_per_frame = int(round(1000000.0 / fps))

    avih = struct.pack(
        '<IIIIIIIIII4I',
        microseconds_per_frame,
        frame_size * fps,
        0,
        0x10,
        total_frames,
        0,
        1,
        frame_size,
        width,
        height,
        0,
        0,
        0,
        0
    )

    strh = struct.pack(
        '<4s4sIHHIIIIIIIIhhhh',
        b'vids',
        b'DIB ',
        0,
        0,
        0,
        0,
        1,
        fps,
        0,
        total_frames,
        frame_size,
        0xFFFFFFFF,
        0,
        0,
        0,
        width,
        height
    )

    raw_row_size = width * 3
    row_padding = (-raw_row_size) % 4
    image_size = (raw_row_size + row_padding) * height

    strf = struct.pack(
        '<IiiHHIIiiII',
        40,
        width,
        height,
        1,
        24,
        0,
        image_size,
        0,
        0,
        0,
        0
    )

    strl = _riff_list(
        b'strl',
        _riff_chunk(b'strh', strh)
        + _riff_chunk(b'strf', strf)
    )

    hdrl = _riff_list(
        b'hdrl',
        _riff_chunk(b'avih', avih)
        + strl
    )

    movi_payload = bytearray()
    index_entries = []
    offset = 4

    for frame_data in frames:
        frame_chunk = _riff_chunk(b'00db', frame_data)
        movi_payload.extend(frame_chunk)

        index_entries.append(
            struct.pack(
                '<4sIII',
                b'00db',
                0x10,
                offset,
                len(frame_data)
            )
        )

        offset += len(frame_chunk)

    movi = _riff_list(b'movi', bytes(movi_payload))
    idx1 = _riff_chunk(b'idx1', b''.join(index_entries))

    riff_payload = b'AVI ' + hdrl + movi + idx1

    with open(avi_path, 'wb') as f:
        f.write(
            b'RIFF'
            + struct.pack('<I', len(riff_payload))
            + riff_payload
        )


# =========================================================
# Real ODB frame export
# =========================================================

def _prepare_frame_dir(frame_dir):
    if not os.path.isdir(frame_dir):
        os.makedirs(frame_dir)
        return

    # Clean only files generated by this exporter.
    for name in os.listdir(frame_dir):
        path = os.path.join(frame_dir, name)
        if os.path.isfile(path) and (
            name.lower().endswith('.png')
            or name == 'frames.txt'
        ):
            os.remove(path)


def _export_real_frames(
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
    avi_path = output_base + '.avi'
    info_path = os.path.join(frame_dir, 'frames.txt')

    parent_dir = os.path.dirname(output_base)
    if parent_dir and not os.path.isdir(parent_dir):
        os.makedirs(parent_dir)

    _prepare_frame_dir(frame_dir)

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

    png_paths = []

    with open(info_path, 'w', encoding='utf-8') as info_file:
        for i in range(len(frames)):
            vp.odbDisplay.setFrame(step=0, frame=i)
            frame_time = frames[i].frameValue

            file_base = os.path.join(
                frame_dir,
                '%s_%04d' % (frame_prefix, i)
            )

            session.printToFile(
                fileName=file_base,
                format=PNG,
                canvasObjects=(vp,)
            )

            png_path = file_base + '.png'
            if not os.path.isfile(png_path):
                raise IOError('PNG export failed: %s' % png_path)

            png_paths.append(png_path)

            info_file.write(
                'Frame %04d   Step Time = %.12g\n'
                % (i, frame_time)
            )

            print(
                '%s: exported Frame %d / %d, Step Time = %s'
                % (
                    variable_label,
                    i,
                    len(frames) - 1,
                    frame_time
                )
            )

    _write_uncompressed_avi(png_paths, avi_path, VIDEO_FPS)

    if not os.path.isfile(avi_path):
        raise IOError('AVI export failed: %s' % avi_path)

    print('=========================================')
    print('%s export finished.' % variable_label)
    print('Frame count = %d' % len(frames))
    print('Video FPS = %d' % VIDEO_FPS)
    print('Frame directory: %s' % frame_dir)
    print('AVI file: %s' % avi_path)
    print('=========================================')


# =========================================================
# 1. Open CAE, create Job, bind 335K.for, and solve
# =========================================================

print('=========================================')
print('Open CAE: %s' % cae_path)
print('User subroutine: %s' % user_subroutine_path)
print('Job: %s' % job_name)
print('=========================================')

mdb = openMdb(cae_path)

# Defensive cleanup in the in-memory MDB in case the same Job name
# already exists in the CAE.
if job_name in mdb.jobs.keys():
    del mdb.jobs[job_name]

mdb.Job(
    name=job_name,
    model='Model-1',
    description='',
    type=ANALYSIS,
    atTime=None,
    waitMinutes=0,
    waitHours=0,
    queue=None,
    memory=90,
    memoryUnits=PERCENTAGE,
    getMemoryFromAnalysis=True,
    explicitPrecision=SINGLE,
    nodalOutputPrecision=SINGLE,
    echoPrint=OFF,
    modelPrint=OFF,
    contactPrint=OFF,
    historyPrint=OFF,
    userSubroutine=user_subroutine_path,
    scratch='',
    resultsFormat=ODB,
    numCpus=1,
    numGPUs=0
)

job = mdb.jobs[job_name]
job.submit()
job.waitForCompletion()

if job.status != COMPLETED:
    raise RuntimeError(
        'Abaqus Job failed, status = %s' % job.status
    )

odb_path = os.path.join(work_dir, job_name + '.odb')
if not os.path.isfile(odb_path):
    raise IOError('ODB not found after completed Job: %s' % odb_path)

print('Abaqus Job completed: %s' % odb_path)


# =========================================================
# 2. Open ODB and reproduce the verified .rpy viewport state
# =========================================================

odb = session.openOdb(name=odb_path)
vp = session.viewports['Viewport: 1']
vp.setValues(displayedObject=odb)
vp.makeCurrent()
vp.maximize()

# The verified replay displayed only PBX material.
leaf = dgo.LeafFromOdbElementMaterials(
    elementMaterials=('PBX', )
)
vp.odbDisplay.displayGroup.replace(leaf=leaf)

# The verified replay exported contours on the deformed shape.
vp.odbDisplay.display.setValues(
    plotState=(CONTOURS_ON_DEF, )
)

# Final camera state immediately before the three verified export scripts
# were run in the supplied Abaqus 2025 .rpy file.
vp.view.setValues(
    nearPlane=397.358,
    farPlane=608.793,
    width=379.342,
    height=190.64,
    cameraPosition=(-335.563, 163.981, -322.396),
    cameraUpVector=(0.298021, 0.907506, 0.296002),
    cameraTarget=(23.2562, 117.183, 23.6633)
)

if STEP_NAME not in odb.steps.keys():
    raise RuntimeError('ODB step not found: %s' % STEP_NAME)

if len(odb.steps[STEP_NAME].frames) == 0:
    raise RuntimeError('%s contains no frames.' % STEP_NAME)

print(
    'ODB real saved frame count = %d'
    % len(odb.steps[STEP_NAME].frames)
)


# =========================================================
# 3. Export the same three result types verified in the .rpy
# =========================================================

# Cure degree: STATEV(1) -> SDV1
_export_real_frames(
    vp=vp,
    odb=odb,
    step_name=STEP_NAME,
    variable_label='SDV1',
    output_position=INTEGRATION_POINT,
    output_base=cure_output_base,
    frame_prefix='Cure_SDV1_frame'
)

# Temperature
_export_real_frames(
    vp=vp,
    odb=odb,
    step_name=STEP_NAME,
    variable_label='NT11',
    output_position=NODAL,
    output_base=temp_output_base,
    frame_prefix='NT11_frame'
)

# Stress: von Mises equivalent stress
_export_real_frames(
    vp=vp,
    odb=odb,
    step_name=STEP_NAME,
    variable_label='S',
    output_position=INTEGRATION_POINT,
    refinement=(INVARIANT, 'Mises'),
    output_base=stress_output_base,
    frame_prefix='Stress_Mises_frame'
)


# =========================================================
# 4. Mark t1 successful only after solver + all postprocessing succeeds
# =========================================================

with open(t1_flag_path, 'w') as f:
    f.write('success')

print('=========================================')
print('t1 finished successfully.')
print('Stress AVI: %s.avi' % stress_output_base)
print('Temperature AVI: %s.avi' % temp_output_base)
print('Cure AVI: %s.avi' % cure_output_base)
print('=========================================')

sys.exit(0)
