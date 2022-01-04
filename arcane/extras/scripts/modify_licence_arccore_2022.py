#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Ajoute le texte de la licence au début des fichiers C/C++
# Le texte est ajouté juste après la première ligne qui
# doit contenir la chaîne 'encode_header'

import os
import io
import argparse
import codecs

# Chaîne pour détecter la licence
licence_detect_string = "// Copyright 2000-2020 IFPEN-CEA"

# Chaîne pour mettre la nouvelle licence
new_licence_date_string = "// Copyright 2000-2022 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)"

# Chaîne à placer pour la licence
licence_header = """//-----------------------------------------------------------------------------
// Copyright 2000-2022 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
"""

def convert_file(filename):
    # Si le nom de fichier contient la chaîne de caractères 'Generated', considère
    # qu'il s'agit d'un fichier généré et ne fais rien.
    if filename.find("Generated") != (-1):
        print(str.format("File {0} is generated. Skip convertion",filename))
        return False
    file_stat = os.stat(filename)
    file_size = file_stat.st_size
    print(str.format("Filename {0} size={1}", filename,file_size))
    # Si le fichier est trop petit, ne fait rien. Cela signifie qu'il n'est pas
    # utilisé où contient juste des includes
    if file_size<500:
        return False
    f = io.open(filename, "r", encoding="utf-8")
    full_content_lines = f.readlines();
    f.close()
    nb_line = len(full_content_lines)
    if (nb_line<10):
        return False;
    # Si le fichier ne contient pas la première ligne spécifiant l'encodage,
    # ne fait rien.
    if full_content_lines[0].find("coding: utf-8") == (-1):
        print(str.format("File '{0}' has no header. Skipping",filename))
        return False

    # Regarde si le fichier contient le nouveau texte de la licence.
    # Si c'est le cas ne fait rien.
    if full_content_lines[2].find(new_licence_date_string) != (-1):
        print(str.format("File '{0}' has valid licence date text",filename))
        return False

    if full_content_lines[2].find(licence_detect_string) != (-1):
        print(str.format("File '{0}' has old licence date text",filename))
    else:
        return False
    print(str.format("Adding licence to file '{0}'",filename))

    utf8_f = open(filename,"w", encoding="utf-8")
    utf8_f.write(full_content_lines[0])   
    utf8_f.write(licence_header)   
    for x in range(18,nb_line):
        utf8_f.write(full_content_lines[x])    
    utf8_f.close()
    return True

def exec_convert():
    parser = argparse.ArgumentParser(description='Update licence text to files.')
    parser.add_argument('files', nargs='+', help='files to convert')
    args = parser.parse_args()
    print(args.files)
    nb_convert = 0;
    converted_files = []
    for x in args.files:
        print(x)
        if convert_file(x):
            nb_convert = nb_convert + 1
            converted_files.append(x)
    print(str.format("NB_CONVERT={0} list={1}",nb_convert,converted_files))

exec_convert()
