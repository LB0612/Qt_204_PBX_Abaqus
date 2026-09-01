# -*- coding: utf-8 -*-

from abaqus import *
from abaqusConstants import *
from caeModules import *
import os
import sys

# t1: submit Abaqus Job with 335K.for and wait for ODB only.

work_dir = '{{ABAQUS_WORK_DIR}}'
cae_path = '{{CAE_FILE_PATH}}'
user_subroutine_path = '{{USER_SUBROUTINE_PATH}}'
job_name = '{{JOB_NAME}}'

t1_flag_path = os.path.join(work_dir, 't1_finished.flag')


def _atomic_write_text(path, content):
    tmp_path = path + '.tmp'
    with open(tmp_path, 'w') as handle:
        handle.write(content)
        handle.flush()
        os.fsync(handle.fileno())
    os.replace(tmp_path, path)


os.chdir(work_dir)

if os.path.isfile(t1_flag_path):
    os.remove(t1_flag_path)

print('=========================================')
print('Open CAE: %s' % cae_path)
print('User subroutine: %s' % user_subroutine_path)
print('Job: %s' % job_name)
print('=========================================')

mdb = openMdb(cae_path)

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

sta_path = os.path.join(work_dir, job_name + '.sta')

if not os.path.isfile(sta_path):
    raise IOError('STA not found after Job: %s' % sta_path)

with open(sta_path, 'r') as handle:
    sta_content = handle.read()

if 'THE ANALYSIS HAS COMPLETED SUCCESSFULLY' not in sta_content:
    raise RuntimeError(
        'Abaqus Job did not complete successfully.'
    )

odb_path = os.path.join(work_dir, job_name + '.odb')
if not os.path.isfile(odb_path):
    raise IOError('ODB not found after completed Job: %s' % odb_path)

print('Abaqus Job completed: %s' % odb_path)

_atomic_write_text(t1_flag_path, 'success')

print('=========================================')
print('t1 solver finished successfully.')
print('ODB: %s' % odb_path)
print('=========================================')

sys.exit(0)
