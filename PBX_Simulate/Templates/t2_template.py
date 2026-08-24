# -*- coding: utf-8 -*-

from abaqus import *
from abaqusConstants import *
from caeModules import *
import displayGroupOdbToolset as dgo
import json
import os
import shutil
import struct
import sys
import zlib

# t2: ODB post-processing only (PNG export + OpenDML AVI).

VIDEO_FPS = 5
STEP_NAME = 'Step-1'
PNG_SIGNATURE = b'\x89PNG\r\n\x1a\n'
MIN_PNG_SIZE = 64
ODML_PLACEHOLDER_SIZE = 32768
RIFF_SEGMENT_LIMIT = 1024 * 1024 * 1024

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
        cure_output_base + '.avi',
        temp_output_base + '.avi',
        stress_output_base + '.avi',
        cure_output_base + '.tmp.avi',
        temp_output_base + '.tmp.avi',
        stress_output_base + '.tmp.avi',
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
            if name.lower().endswith('.tmp.avi'):
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
    with open(path, 'rb') as handle:
        data = handle.read()

    if data[:8] != PNG_SIGNATURE:
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


def _align_chunk_end(position):
    if position % 2:
        return position + 1
    return position


class _OpenDmlAviWriter(object):
    def __init__(self, path, fps):
        self._path = path
        self._fps = fps
        self._microsec_per_frame = int(round(1000000.0 / fps))
        self._file = open(path, 'wb')

        self._width = 0
        self._height = 0
        self._frame_size = 0
        self._total_frames = 0

        self._segment_index_entries = []
        self._segment_meta = []

        self._avih_total_frames_pos = 0
        self._strh_length_pos = 0
        self._dmlh_total_frames_pos = 0
        self._odml_placeholder_pos = 0
        self._odml_placeholder_size = ODML_PLACEHOLDER_SIZE

        self._segment_riff_start = 0
        self._segment_riff_size_pos = 0
        self._segment_movi_size_pos = 0
        self._segment_movi_data_start = 0

    def write_frame(self, frame_data, width, height):
        if self._total_frames == 0:
            self._width = width
            self._height = height
            self._frame_size = len(frame_data)
            self._begin_segment(is_first=True)
        else:
            if width != self._width or height != self._height:
                raise RuntimeError('All PNG frames must have the same size.')
            if len(frame_data) != self._frame_size:
                raise RuntimeError('Inconsistent AVI frame size.')

            if self._segment_payload_size() + self._frame_chunk_size(len(frame_data)) > RIFF_SEGMENT_LIMIT:
                self._finish_segment()
                self._begin_segment(is_first=False)

        chunk = _riff_chunk(b'00db', frame_data)
        offset_in_movi = self._file.tell() - self._segment_movi_data_start
        self._file.write(chunk)
        self._segment_index_entries.append((offset_in_movi, len(frame_data)))
        self._total_frames += 1

    def close(self):
        if self._total_frames == 0:
            raise RuntimeError('No PNG frames; AVI cannot be created.')

        self._finish_segment()
        self._patch_headers()

        if len(self._segment_meta) > 1:
            self._write_super_index()

        self._file.close()

    def _frame_chunk_size(self, frame_size):
        chunk = _riff_chunk(b'00db', b'\x00' * frame_size)
        return len(chunk)

    def _segment_payload_size(self):
        return self._file.tell() - self._segment_riff_start - 8

    def _begin_segment(self, is_first):
        self._segment_riff_start = self._file.tell()
        self._file.write(b'RIFF')
        self._segment_riff_size_pos = self._file.tell()
        self._file.write(struct.pack('<I', 0))
        self._file.write(b'AVI ' if is_first else b'AVIX')

        if is_first:
            self._write_hdrl()

        self._file.write(b'LIST')
        self._segment_movi_size_pos = self._file.tell()
        self._file.write(struct.pack('<I', 0))
        self._file.write(b'movi')
        self._segment_movi_data_start = self._file.tell()
        self._segment_index_entries = []

    def _write_hdrl(self):
        avih = struct.pack(
            '<IIIIIIIIII4I',
            self._microsec_per_frame,
            self._frame_size * self._fps,
            0,
            0x910,
            0,
            0,
            1,
            self._frame_size,
            self._width,
            self._height,
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
            self._fps,
            0,
            0,
            self._frame_size,
            0xFFFFFFFF,
            0,
            0,
            0,
            self._width,
            self._height
        )

        raw_row_size = self._width * 3
        row_padding = (-raw_row_size) % 4
        image_size = (raw_row_size + row_padding) * self._height

        strf = struct.pack(
            '<IiiHHIIiiII',
            40,
            self._width,
            self._height,
            1,
            24,
            0,
            image_size,
            0,
            0,
            0,
            0
        )

        avih_chunk = _riff_chunk(b'avih', avih)
        strl = _riff_list(
            b'strl',
            _riff_chunk(b'strh', strh)
            + _riff_chunk(b'strf', strf)
        )
        dmlh_chunk = _riff_chunk(b'dmlh', struct.pack('<I', 0))
        odml_placeholder = _riff_chunk(
            b'JUNK',
            b'\x00' * (self._odml_placeholder_size - 8)
        )

        hdrl_start = self._file.tell()
        hdrl_body = avih_chunk + strl + dmlh_chunk + odml_placeholder
        hdrl = _riff_list(b'hdrl', hdrl_body)
        body_start = hdrl_start + 12

        self._avih_total_frames_pos = body_start + 8 + 16
        self._strh_length_pos = body_start + len(avih_chunk) + 12 + 8 + 32
        dmlh_pos = body_start + len(avih_chunk) + len(strl)
        self._dmlh_total_frames_pos = dmlh_pos + 8
        self._odml_placeholder_pos = dmlh_pos + len(dmlh_chunk)

        self._file.write(hdrl)

    def _write_standard_index(self):
        entries = []
        for offset, size in self._segment_index_entries:
            entries.append(
                struct.pack(
                    '<II',
                    offset | 0x80000000,
                    size
                )
            )

        body = struct.pack(
            '<HBBI4s3I',
            2,
            0,
            0,
            len(self._segment_index_entries),
            b'00db',
            0,
            0,
            0
        ) + b''.join(entries)

        return _riff_chunk(b'indx', body)

    def _finish_segment(self):
        movi_end = self._file.tell()
        movi_body_size = movi_end - (self._segment_movi_size_pos + 4)
        self._file.seek(self._segment_movi_size_pos)
        self._file.write(struct.pack('<I', movi_body_size + 4))
        self._file.seek(movi_end)

        indx_start = self._file.tell()
        indx_chunk = self._write_standard_index()
        self._file.write(indx_chunk)
        indx_end = self._file.tell()

        riff_end = indx_end
        riff_size = riff_end - self._segment_riff_start - 8
        self._file.seek(self._segment_riff_size_pos)
        self._file.write(struct.pack('<I', riff_size))
        self._file.seek(riff_end)

        self._segment_meta.append({
            'indx_offset': indx_start,
            'indx_size': indx_end - indx_start,
            'entries': len(self._segment_index_entries),
        })

    def _patch_headers(self):
        total = self._total_frames
        self._file.seek(self._avih_total_frames_pos)
        self._file.write(struct.pack('<I', total))

        self._file.seek(self._strh_length_pos)
        self._file.write(struct.pack('<I', total))

        self._file.seek(self._dmlh_total_frames_pos)
        self._file.write(struct.pack('<I', total))

        self._file.seek(0, os.SEEK_END)

    def _write_super_index(self):
        entries = []
        for meta in self._segment_meta:
            offset = meta['indx_offset']
            entries.append(
                struct.pack(
                    '<IIIIII',
                    0,
                    0x10,
                    offset & 0xFFFFFFFF,
                    (offset >> 32) & 0xFFFFFFFF,
                    meta['indx_size'],
                    meta['entries']
                )
            )

        body = struct.pack(
            '<HBBI4s3I',
            6,
            0,
            1,
            len(entries),
            b'00db',
            0,
            0,
            0
        ) + b''.join(entries)

        odml = _riff_list(b'odml', _riff_chunk(b'indx', body))

        if len(odml) > self._odml_placeholder_size:
            raise RuntimeError(
                'OpenDML super index exceeds reserved header space.'
            )

        padding_size = self._odml_placeholder_size - len(odml)
        if padding_size > 0:
            odml += b'\x00' * padding_size

        self._file.seek(self._odml_placeholder_pos)
        self._file.write(odml)
        self._file.seek(0, os.SEEK_END)


def _count_avi_frames(path):
    dmlh_total = None
    avih_total = None
    db_chunks = 0

    with open(path, 'rb') as handle:
        while True:
            riff_header = handle.read(12)
            if len(riff_header) < 12:
                break

            if riff_header[0:4] != b'RIFF':
                break

            riff_size, = struct.unpack('<I', riff_header[4:8])
            form_type = riff_header[8:12]
            if form_type not in (b'AVI ', b'AVIX'):
                break

            riff_end = handle.tell() + riff_size - 4
            if riff_end < handle.tell():
                break

            while handle.tell() < riff_end:
                chunk_header = handle.read(8)
                if len(chunk_header) < 8:
                    break

                fourcc, chunk_size = struct.unpack('<4sI', chunk_header)
                chunk_start = handle.tell()

                if fourcc == b'LIST':
                    list_type = handle.read(4)
                    if list_type == b'hdrl':
                        list_end = chunk_start + chunk_size
                        while handle.tell() < list_end:
                            child_header = handle.read(8)
                            if len(child_header) < 8:
                                break
                            child_fourcc, child_size = struct.unpack(
                                '<4sI',
                                child_header
                            )
                            child_start = handle.tell()

                            if child_fourcc == b'avih':
                                data = handle.read(child_size)
                                if len(data) >= 20:
                                    avih_total = struct.unpack(
                                        '<I',
                                        data[16:20]
                                    )[0]
                            elif child_fourcc == b'dmlh':
                                data = handle.read(child_size)
                                if len(data) >= 4:
                                    dmlh_total = struct.unpack(
                                        '<I',
                                        data[0:4]
                                    )[0]
                            else:
                                handle.seek(child_start + child_size)

                            handle.seek(
                                _align_chunk_end(child_start + child_size)
                            )
                    else:
                        handle.seek(chunk_start + chunk_size)
                elif fourcc == b'00db':
                    db_chunks += 1
                    handle.seek(chunk_start + chunk_size)
                else:
                    handle.seek(chunk_start + chunk_size)

                handle.seek(_align_chunk_end(chunk_start + chunk_size))

            handle.seek(riff_end)

    if dmlh_total is not None and dmlh_total > 0:
        return dmlh_total
    if avih_total is not None and avih_total > 0:
        return avih_total
    if db_chunks > 0:
        return db_chunks

    raise RuntimeError('Unable to determine AVI frame count: %s' % path)


def _avi_is_valid(avi_path, expected_frames):
    if not os.path.isfile(avi_path):
        return False

    try:
        return _count_avi_frames(avi_path) == expected_frames
    except Exception as error:
        print('[POST] Existing AVI invalid: %s (%s)' % (avi_path, error))
        return False


def _generate_avi(frame_dir, frame_prefix, output_avi, expected_frames):
    if _avi_is_valid(output_avi, expected_frames):
        print('[POST] Skip existing valid AVI: %s' % output_avi)
        return expected_frames, os.path.getsize(output_avi)

    tmp_avi = output_avi + '.tmp.avi'
    if os.path.isfile(tmp_avi):
        os.remove(tmp_avi)

    writer = _OpenDmlAviWriter(tmp_avi, VIDEO_FPS)

    for index in range(expected_frames):
        png_path = _frame_png_path(frame_dir, frame_prefix, index)
        width, height, frame_data = _make_avi_frame(png_path)
        writer.write_frame(frame_data, width, height)

    writer.close()

    actual_frames = _count_avi_frames(tmp_avi)
    if actual_frames != expected_frames:
        if os.path.isfile(tmp_avi):
            os.remove(tmp_avi)
        raise RuntimeError(
            'AVI frame mismatch for %s: expected %d, got %d'
            % (output_avi, expected_frames, actual_frames)
        )

    os.replace(tmp_avi, output_avi)
    video_bytes = os.path.getsize(output_avi)
    print(
        '[POST] AVI generated: %s (%d frames, %d bytes)'
        % (output_avi, actual_frames, video_bytes)
    )
    return actual_frames, video_bytes


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

# Phase B: generate AVI videos only after all PNG sets are complete.
cure_video_frames, cure_video_bytes = _generate_avi(
    cure_output_base + '_frames',
    'Cure_SDV1_frame',
    cure_output_base + '.avi',
    expected_frames
)
temp_video_frames, temp_video_bytes = _generate_avi(
    temp_output_base + '_frames',
    'NT11_frame',
    temp_output_base + '.avi',
    expected_frames
)
stress_video_frames, stress_video_bytes = _generate_avi(
    stress_output_base + '_frames',
    'Stress_Mises_frame',
    stress_output_base + '.avi',
    expected_frames
)

post_sha = _post_sha256()
manifest = {
    'version': 2,
    'postSha256': post_sha,
    'step': STEP_NAME,
    'odbFrames': expected_frames,
    'curePngFrames': cure_count,
    'temperaturePngFrames': temp_count,
    'stressPngFrames': stress_count,
    'cureVideoFrames': cure_video_frames,
    'temperatureVideoFrames': temp_video_frames,
    'stressVideoFrames': stress_video_frames,
    'cureVideoBytes': cure_video_bytes,
    'temperatureVideoBytes': temp_video_bytes,
    'stressVideoBytes': stress_video_bytes,
    'videoFps': VIDEO_FPS,
}

_atomic_write_json(manifest_path, manifest)
_atomic_write_text(t2_flag_path, 'success')

print('=========================================')
print('t2 post-processing finished successfully.')
print('Manifest: %s' % manifest_path)
print('Cure AVI: %s.avi' % cure_output_base)
print('Temperature AVI: %s.avi' % temp_output_base)
print('Stress AVI: %s.avi' % stress_output_base)
print('=========================================')

sys.exit(0)
