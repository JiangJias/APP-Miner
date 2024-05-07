#!/usr/bin/python
from __future__ import with_statement

import ast
import copy
import multiprocessing
import os
import re
import sys
import shutil
import time
import math
import random

from multiprocessing import cpu_count, Pool, Manager
from argparse import ArgumentParser
from graphviz import Digraph
from decimal import *

CONFIDENCE_THRESHOLD = Decimal('0.9')
MIN_SUPPORT = 10
PROCESSTOTAL = int(cpu_count() / 4)


def statistic(API):
    locationsize = 0
    locationmap = {}
    pathsize = 0
    pathmap = {}
    for root, dirs, files in os.walk('tmp/' + project + '/dfa/' + API):
        for file in files:
            if not file.isdigit():
                continue
            print(file)
            with open(os.path.join(root, file), 'r') as file_read:
                for line in file_read:
                    output = {'path': []}
                    APIPath = ast.literal_eval(line)
                    tmploc = str({'file': APIPath['file'], 'function': APIPath['function']})
                    if tmploc not in locationmap:
                        locationmap[tmploc] = locationsize
                        locationsize += 1
                    output['location'] = locationmap[tmploc]
                    for path in APIPath['path']:
                        AP1 = path['AP']
                        for parameter1 in path['parameter']:
                            for next in path['next']:
                                AP2 = next['AP']
                                if AP1 == AP2: # Remove circle nodes
                                    continue
                                for parameter2 in next['parameter']:
                                    tmppath = str({'AP1': AP1, 'parameter1': parameter1, 'AP2': AP2, 'parameter2': parameter2})
                                    if tmppath not in pathmap:
                                        pathmap[tmppath] = pathsize
                                        pathsize += 1
                                    output['path'].append(pathmap[tmppath])
                    with open('result/' + project + '/statistic/' + API, 'a') as file_write:
                        print(output, file=file_write)
    with open('result/' + project + '/statistic_location/' + API, 'a') as file_write:
        print(dict(zip(locationmap.values(), locationmap.keys())), file=file_write)
    with open('result/' + project + '/statistic_path/' + API, 'a') as file_write:
        print(dict(zip(pathmap.values(), pathmap.keys())), file=file_write)


def prune(file_name):
    APP = {}
    count = {}
    total = 0
    tracked = []
    locationsize = 0
    locationmap = {}
    pathsize = 0
    pathmap = {}
    with open('result/' + project + '/statistic/' + file_name, 'r') as file_read:
        for line in file_read:
            APIPath = ast.literal_eval(line)
            location = APIPath['location']
            if location not in tracked:
                tracked.append(location)
                total += 1
            for path in APIPath['path']:
                if path not in count:
                    count[path] = []
                if location not in count[path]:
                    count[path].append(location)

    threshold = max(math.ceil(CONFIDENCE_THRESHOLD * total), MIN_SUPPORT)
    for path in count:
        if len(count[path]) >= threshold:
           pathmap[path] = pathsize
           pathsize += 1
           APP[pathmap[path]] = []
           for location in count[path]:
                if location not in locationmap:
                    locationmap[location] = locationsize
                    locationsize += 1
                APP[pathmap[path]].append(locationmap[location])
    if not APP:
        return

    with open('result/' + project + '/APPinput/' + file_name, 'a') as file_write:
        for indexpath in APP:
            print(indexpath, file = file_write, sep = '', end = '')
            for indexloc in APP[indexpath]:
                print(',', indexloc, file = file_write, sep = '', end = '')
            print(file = file_write)
    downwardClosure = 'result/' + project + '/downwardClosure/' + file_name
    os.system('sed "s/PATHSIZE/' + str(pathsize) + '/g" downwardClosure.c > ' + downwardClosure + '.c')
    os.system('sed -i "s/LOCATIONSIZE/' + str(locationsize) + '/g" ' + downwardClosure + '.c')
    os.system('g++ ' + downwardClosure + '.c -o ' + downwardClosure + '.o')
    os.system('./' + downwardClosure + '.o ' + project + ' ' + file_name + ' ' + str(threshold))
    APP.clear()

    with open('result/' + project + '/APPoutput/' + file_name, 'r') as file_read:
        for line in file_read:
            APP = ast.literal_eval(line)
            output = {'support': len(APP['location']), 'path': [], 'location': []}
            for indexpath in APP['path']:
                output['path'].append(getDictKey(pathmap, indexpath))
            for indexlocation in APP['location']:
                output['location'].append(getDictKey(locationmap, indexlocation))
            with open('result/' + project + '/APP/' + file_name, 'a') as file_write:
                print(output, file=file_write)


def violate(file_name):
    with open('result/' + project + '/statistic/' + file_name, 'r') as file_read1:
        for line1 in file_read1:
            APIPath = ast.literal_eval(line1)
            satisfy = False
            maxSupport = 0
            allAPP = []
            
            with open('result/' + project + '/APP/' + file_name, 'r') as file_read2:
                for line2 in file_read2:
                    APP = ast.literal_eval(line2)
                    flag = True
                    for path in APP['path']:
                        if path not in APIPath['path']:
                            if APP['support'] > maxSupport:
                                maxSupport = APP['support']
                                maxAPP = APP
                            allAPP.append(APP)
                            flag = False
                            break
                    if flag:
                        satisfy = True
                        break
            if not satisfy and allAPP:
                with open('result/' + project + '/statistic_location/' + file_name, 'r') as file_read:
                    locationmap = ast.literal_eval(file_read.read())
                with open('result/' + project + '/statistic_path/' + file_name, 'r') as file_read:
                    pathmap = ast.literal_eval(file_read.read())
                output = []
                for unsatisfyAPP in allAPP:
                    paths = []
                    for path in unsatisfyAPP['path']:
                        paths.append(ast.literal_eval(pathmap[path]))
                    output.append(paths)
                tmp = {'location': locationmap[APIPath['location']], 'API': file_name, 'APP': {}}
                tmp['APP']['support'] = maxAPP['support']
                tmp['APP']['path'] = str(output)
                tmp['APP']['location'] = locationmap[maxAPP['location'][0]]
                with open('result/' + project + '/violation/' + file_name, 'a') as file_write:
                    print(tmp, file=file_write)


def sort_violate(violation_list):
    violation_order = sorted(violation_list, key=lambda x: (
        x['APP']['support'], x['API'], str(x['APP']['path']), x['location']), reverse=True)
    return violation_order


def isSatisfy(APIPath, path, pathmap):
    for indexpath in APIPath['path']:
        if str(path) == str(pathmap[indexpath]):
            return True
    return False


def isContain(tmp, group):
    for path in tmp:
        if path not in group:
            return False
    return True


def getDictKey(Dict, value):
    for k, v in Dict.items():
        if v == value:
            return k


def main(step):
    if step == 'parseIR':
        print(step)
        time_start = time.perf_counter()
        os.system('python2 ../deadline/work/main.py -i="' + configCmd + '" -a="' + project + '" config')
        os.system('python2 ../deadline/work/main.py -i="' + buildCmd + '" -a="' + project + '" build')
        os.system('python2 ../deadline/work/main.py -a="' + project + '" parse')
        os.system('python2 ../deadline/work/main.py -a="' + project + '" irgen')
        time_end = time.perf_counter()
        time_sum = time_end - time_start
        print('parseIR time cost:', time_sum, 's')
        step = 'preprocess'

    if step == 'preprocess':
        print(step)
        time_start = time.perf_counter()
        os.system('../preprocessor/analyzer/batch.sh ' + source + ' tmp/' + project + '/bc.list')
        os.system('../preprocessor/analyzer/build/lib/kanalyzer ' + source + ' @tmp/' + project + '/bc.list')
        time_end = time.perf_counter()
        time_sum = time_end - time_start
        print('preprocess time cost:', time_sum, 's')
        step = 'statistic'

    if step == 'statistic':
        print(step)
        time_start = time.perf_counter()
        if os.path.exists('result/' + project + '/statistic'):
            shutil.rmtree('result/' + project + '/statistic')
        os.makedirs('result/' + project + '/statistic')
        if os.path.exists('result/' + project + '/statistic_location'):
            shutil.rmtree('result/' + project + '/statistic_location')
        os.makedirs('result/' + project + '/statistic_location')
        if os.path.exists('result/' + project + '/statistic_path'):
            shutil.rmtree('result/' + project + '/statistic_path')
        os.makedirs('result/' + project + '/statistic_path')
        pool = Pool(PROCESSTOTAL)
        for root, dirs, files in os.walk('tmp/' + project + '/dfa'):
            for dir in dirs:
                pool.apply_async(statistic, args=(dir, ))
        pool.close()
        pool.join()
        time_end = time.perf_counter()
        time_sum = time_end - time_start
        print('statistic time cost:', time_sum, 's')
        step = 'prune'

    if step == 'prune':
        print(step)
        time_start = time.perf_counter()
        if os.path.exists('result/' + project + '/downwardClosure'):
            shutil.rmtree('result/' + project + '/downwardClosure')
        os.makedirs('result/' + project + '/downwardClosure')
        if os.path.exists('result/' + project + '/APPinput'):
            shutil.rmtree('result/' + project + '/APPinput')
        os.makedirs('result/' + project + '/APPinput')
        if os.path.exists('result/' + project + '/APPoutput'):
            shutil.rmtree('result/' + project + '/APPoutput')
        os.makedirs('result/' + project + '/APPoutput')
        if os.path.exists('result/' + project + '/APP'):
            shutil.rmtree('result/' + project + '/APP')
        os.makedirs('result/' + project + '/APP')
        APPTimeout = []
        if os.path.exists('result/' + project + '/APPTimeout'):
            with open('result/' + project + '/APPTimeout') as file_read:
                for line in file_read:
                    APPTimeout.append(line.strip())
        pool = Pool(PROCESSTOTAL)
        for root, dirs, files in os.walk('result/' + project + '/statistic'):
            for file in files:
                if file.endswith('.swp') or file in APPTimeout:
                    continue
                pool.apply_async(prune, args=(file, ))
        pool.close()
        pool.join()
        time_end = time.perf_counter()
        time_sum = time_end - time_start
        print('prune time cost:', time_sum, 's')
        step = 'violate'

    if step == 'violate':
        print(step)
        time_start = time.perf_counter()
        if os.path.exists('result/' + project + '/violation'):
            shutil.rmtree('result/' + project + '/violation')
        os.makedirs('result/' + project + '/violation')
        pool = Pool(PROCESSTOTAL)
        for root, dirs, files in os.walk('result/' + project + '/APP'):
            for file in files:
                if file.endswith('.swp'):
                    continue
                if 'debug' in file or 'test' in file:
                    continue
                pool.apply_async(violate, args=(file, ))
        pool.close()
        pool.join()
        time_end = time.perf_counter()
        time_sum = time_end - time_start
        print('violate time cost:', time_sum, 's')
        step = 'sort'

    if step == 'sort':
        print(step)
        if os.path.exists('result/' + project + '/order'):
            os.remove('result/' + project + '/order')
        violation = []
        for root, dirs, files in os.walk('result/' + project + '/violation'):
            for file in files:
                if file.endswith('.swp'):
                    continue
                location = []
                with open(os.path.join(root, file), 'r') as file_read:
                    for line in file_read:
                        tmp = ast.literal_eval(line)
                        # remove duplicate location
                        if tmp['location'] in location:
                            continue
                        location.append(tmp['location'])
                        violation.append(tmp)
        violation_order = sort_violate(violation)
        with open('result/' + project + '/order', 'a') as file_write:
            for item in violation_order:
                print(item, file=file_write)
        step = 'xml'

    if step == 'xml':
        print(step)
        if os.path.exists('result/' + project + '/xml'):
            os.remove('result/' + project + '/xml')
        with open('result/' + project + '/order', 'r') as file_read:
            i = 0
            API = []
            for line in file_read:
                violation = ast.literal_eval(line)
                if violation['API'] not in API:
                    API.append(violation['API'])
                with open('result/' + project + '/xml', 'a') as file_write:
                    i = i + 1
                    print(i, '\t', violation['APP']['location'], '\t', violation['API'], '\t', len(API), '\t', violation['APP']['path'], '\t', violation['APP']['support'], '\t', violation['location'], sep = '', file=file_write)


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('-p', '--process', type = str, default = 'parseIR')
    parser.add_argument('-s', '--source', type = str, required = True)
    parser.add_argument('-c', '--configCmd', type = str, default = 'make allyesconfig')
    parser.add_argument('-b', '--buildCmd', type = str, default = 'make')

    args = parser.parse_args()
    process = args.process
    source = args.source
    configCmd = args.configCmd
    buildCmd = args.buildCmd

    project = os.path.basename(os.path.normpath(source))
    source = os.path.split(os.path.split(os.path.split(source)[0])[0])[0] + '/bcfs/' + project + '/'

    if not os.path.exists('tmp'):
        os.makedirs('tmp')
    if not os.path.exists('tmp/' + project):
        os.makedirs('tmp/' + project)
    if not os.path.exists('result/' + project):
        os.makedirs('result/' + project)
    main(process)
