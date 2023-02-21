#!/bin/python3
import multiprocessing
import os
from os import path
import signal
import subprocess
import sys

# Relative to Userland directory
PATHS_TO_SEARCH = [
    'Applications/Assistant',
    'Applications/Browser',
    'Applications/Spreadsheet',
    'Applications/TextEditor',
    'DevTools/HackStudio',
    'Libraries/LibJS',
    'Libraries/LibMarkdown',
    'Libraries/LibWeb',
    'Services/WebContent',
]

subprocess.run(['cmake', '--build', './build'], check=True)

serenity_path = path.abspath(path.join(__file__, '..', '..', '..', '..', '..', '..'))
userland_path = path.join(serenity_path, 'Userland')
include_path = path.join(serenity_path, 'Build', 'x86_64clang', 'Root', 'usr', 'include')
compile_commands_path = path.join(serenity_path, 'Build', 'x86_64clang', 'compile_commands.json')

if not path.exists(compile_commands_path):
    print(f'Could not find compile_commands.json in {path.basename(compile_commands_path)}')
    exit(1)

paths = []

for containing_path in PATHS_TO_SEARCH:
    for root, dirs, files in os.walk(path.join(userland_path, containing_path)):
        for file in files:
            if file.endswith('.h') or file.endswith('.cpp'):
                paths.append(path.join(root, file))


def thread_init():
    signal.signal(signal.SIGINT, signal.SIG_IGN)


def thread_execute(file_path):
    args = [
        './build/libjs-gc-verifier',
        '--extra-arg',
        f'-I{include_path}',
        '--extra-arg',
        '-DUSING_AK_GLOBALLY=1',  # To avoid errors about USING_AK_GLOBALLY not being defined at all
        '-p',
        compile_commands_path,
        file_path
    ]
    proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    sys.stdout.buffer.write(proc.communicate()[0])
    sys.stdout.buffer.flush()


with multiprocessing.Pool(processes=multiprocessing.cpu_count() - 2, initializer=thread_init) as pool:
    try:
        pool.map(thread_execute, paths)
    except KeyboardInterrupt:
        pool.terminate()
        pool.join()


