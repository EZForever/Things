#!/usr/bin/env python3
import os
import sys
import getopt
import configparser

import requests

from typing import Any

USAGE = '''Usage: mndb.py [-h] [-v] [-k <apikey>] [-n <items>] <queries>...
query Windows magic numbers from MagNumDB (https://www.magnumdb.com/).

    -h, --help     display this help and exit
    -v, --verbose  show verbose results (e.g. GUID formats)
    -k, --apikey   specify API key (default to read from ~/.config/mndb.conf)
    -n, --items    maximum number of items per query (default unlimited)

mndb.conf takes the form of:

    [mndb]
    apikey=<your API key here>

to obtain an API key, please contact the maintainers of MagNumDB.
'''

def print_value_item(verbose: bool, item: dict[str, Any]) -> None:
    if 'HexValue' in item and 'SignedValue' in item:
        print(f'\thex {item["HexValue"]} / signed {item["SignedValue"]} / unsigned {item["Value"]}')
    else:
        print(f'\t{item["Value"]}')
    
    # XXX: Certain GUID entries is of ValueItem
    if verbose and 'GuidFormats' in item:
        print('\n'.join('\t' + x for x in item['GuidFormats'].split('<br/>')))

def main(argv: list[str]) -> int:
    try:
        opts, args = getopt.getopt(argv[1 : ], 'hvk:n:', ['help', 'verbose', 'apikey=', 'items='])
    except getopt.GetoptError as exc:
        print(f'{argv[0]}: argument parse error: {exc}', file = sys.stderr)
        return 1

    if len(args) == 0:
        print(USAGE, file = sys.stderr)
        return 0

    verbose = False
    apikey: str = None
    items = -1
    for opt, arg in opts:
        if opt in ('-h', '--help'):
            print(USAGE, file = sys.stderr)
            return 0
        elif opt in ('-v', '--verbose'):
            verbose = True
        elif opt in ('-k', '--apikey'):
            apikey = arg
        elif opt in ('-n', '--items'):
            try:
                items = int(arg)
            except ValueError:
                print(f'{argv[0]}: invalid max item value', file = sys.stderr)
                return 1
        else:
            print(f'{argv[0]}: ignoring unknown option {opt}', file = sys.stderr)

    if apikey is None:
        try:
            conf = configparser.ConfigParser()
            conf.read(os.path.expanduser('~/.config/mndb.conf'))
            apikey = conf['mndb']['apikey']
        except Exception as exc:
            print(f'{argv[0]}: unable to read API key: {exc}', file = sys.stderr)
            return 2

    session = requests.Session()
    for arg in args:
        try:
            req = session.get('https://www.magnumdb.com/api.aspx', params = {
                'q': arg,
                'key': apikey
            })
        except Exception as exc:
            print(f'{argv[0]}: unable to send query: {exc}', file = sys.stderr)
            continue

        if req.status_code != 200:
            print(f'{argv[0]}: server response error {req.status_code}: {req.text}', file = sys.stderr)
            continue
        
        resp = req.json()
        total = resp["TotalHits"]
        print(f'// {total} match(es) found for "{resp["OriginalText"]}", {min(total, items) if items >= 0 else total} shown')
        print()
        
        for i, item in enumerate(resp['Items']):
            if items >= 0 and i >= items:
                break
            
            print(f'{item["Title"]:<48}\t{item["FileName"]}')
            
            item_type = item['Type']
            if item_type == 'ValueItem':
                print(f'\t{item["ValueType"]} value')
                print_value_item(verbose, item)
            elif item_type == 'EnumValueItem':
                print(f'\t{item["ValueType"]} member of enum {item["Parent"]}')
                print_value_item(verbose, item)
            elif item_type == 'GuidItem':
                guid_type = item['GuidType'] if 'GuidType' in item else 'Unknown'
                print(f'\t{guid_type} GUID of type {item["ValueType"]}')
                print_value_item(verbose, item)
            elif item_type == 'EnumItem':
                print(f'\tenum type')
                if not verbose:
                    print(f'\t// enable verbose mode to get values from source')
            else:
                print(f'\t{item["Value"]}') # XXX: At least print the value out
            
            if verbose:
                print(f'// {item["DisplayFilePath"]}')
                for condition in item['Conditions']:
                    print(f'// - {condition}')
                print('\n'.join(item['Source'].splitlines()))
            
            print()

    return 0

if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv))
    except KeyboardInterrupt:
        sys.exit(130)

