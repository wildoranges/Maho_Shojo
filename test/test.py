#!/usr/bin/env python3
import subprocess
import os

IRBuild_ptn = '"{}" "-nocheck" "-emit-mir" "-o" "{}" "{}"'
ExeGen_ptn = '"clang" "{}" "-o" "{}" "{}" "../lib/lib.c"'
Exe_ptn = '"{}"'

def eval(EXE_PATH, TEST_BASE_PATH, timeout, optimization):
    print('===========TEST START===========')
    for case in testcases:
        print('Case %s:' % case, end='')
        TEST_PATH = TEST_BASE_PATH + case
        SY_PATH = TEST_BASE_PATH + case + '.sy'
        LL_PATH = TEST_BASE_PATH + case + '.ll'
        INPUT_PATH = TEST_BASE_PATH + case + '.in'
        OUTPUT_PATH = TEST_BASE_PATH + case + '.out'
        need_input = testcases[case]

        IRBuild_result = subprocess.run(IRBuild_ptn.format(EXE_PATH, LL_PATH, SY_PATH), shell=True, stderr=subprocess.PIPE)
        if IRBuild_result.returncode == 0:
            input_option = None
            if need_input:
                with open(INPUT_PATH, "rb") as fin:
                    input_option = fin.read()

            try:
                subprocess.run(ExeGen_ptn.format(optimization, TEST_PATH, LL_PATH), shell=True, stderr=subprocess.PIPE)
                result = subprocess.run(Exe_ptn.format(TEST_PATH), shell=True, input=input_option, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
                out = result.stdout.split(b'\n')
                if result.returncode != b'':
                    out.append(str(result.returncode).encode())
                for i in range(len(out)-1, -1, -1):
                    if out[i] == b'':
                        out.remove(b'')
                with open(OUTPUT_PATH, "rb") as fout:
                    i = 0
                    Success_flag = True
                    for line in fout.readlines():
                        line = line.strip(b'\n')
                        if line == '':
                            continue
                        if out[i] != line:
                            Success_flag == False
                            print(result.stdout[:100], result.returncode, out[i][:100], line[:100], end='')
                            print('\t\033[31mWrong Answer\033[0m')
                        i = i + 1
                    if Success_flag == True:
                        print('\t\033[32mSuccess\033[0m')
                print(result.stderr.decode())
            except Exception as _:
                print(_, end='')
                print('\t\033[31mCodeGen or CodeExecute Fail\033[0m')
            finally:
                subprocess.call(["rm", "-rf", TEST_PATH, TEST_PATH])
                subprocess.call(["rm", "-rf", TEST_PATH, TEST_PATH + ".o"])
                subprocess.call(["rm", "-rf", TEST_PATH, TEST_PATH + ".ll"])

        else:
            print(IRBuild_result.stderr)
            print('\t\033[31mIRBuilder Fail\033[0m')

    print('============TEST END============')


if __name__ == "__main__":
    # you should only revise this
    TEST_BASE_PATH = './performance_test2021_pre/'
    timeout = 50             # generally less than 50s
    optimization = "-O0"     # -O0 -O1 -O2 -O3 -O4(currently = -O3) -Ofast
    # you should only revise this
    testcases = {}  # { name: need_input }
    EXE_PATH = os.path.abspath('../build/compiler')
    testcase_list = list(map(lambda x: x.split('.'), os.listdir(TEST_BASE_PATH)))
    testcase_list.sort()
    for i in range(len(testcase_list)-1, -1, -1):
        if len(testcase_list[i]) == 1:
            testcase_list.remove(testcase_list[i])
    for i in range(len(testcase_list)):
        testcases[testcase_list[i][0]] = False
    for i in range(len(testcase_list)):
        testcases[testcase_list[i][0]] = testcases[testcase_list[i][0]] | (testcase_list[i][1] == 'in')
    eval(EXE_PATH, TEST_BASE_PATH, timeout=timeout, optimization=optimization)
