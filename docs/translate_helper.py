#!/usr/bin/env python3
"""
Helper script to add language switchers to markdown files
"""
import os
import re

def add_language_switcher_to_english(filename):
    """Add language switcher to English markdown file"""
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check if language switcher already exists
    if '**Language:**' in content or '> **Language:**' in content:
        print(f"  ✓ Language switcher already exists in {filename}")
        return False
    
    # Get base name for Korean file
    base_name = filename.replace('.md', '')
    korean_file = f"{base_name}_KO.md"
    
    # Find the first heading
    lines = content.split('\n')
    insert_pos = 0
    
    for i, line in enumerate(lines):
        if line.startswith('# '):
            insert_pos = i + 1
            break
    
    # Insert language switcher after first heading
    switcher = f'\n> **Language:** **English** | [한국어]({os.path.basename(korean_file)})\n'
    lines.insert(insert_pos, switcher)
    
    # Write back
    with open(filename, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))
    
    print(f"  ✓ Added language switcher to {filename}")
    return True

def main():
    # Get all English markdown files (excluding Korean versions)
    md_files = [f for f in os.listdir('.') if f.endswith('.md') and not f.endswith('_KO.md')]
    md_files.sort()
    
    print(f"Processing {len(md_files)} English markdown files...")
    print()
    
    modified_count = 0
    for md_file in md_files:
        if add_language_switcher_to_english(md_file):
            modified_count += 1
    
    print()
    print(f"Summary: Modified {modified_count} files, {len(md_files) - modified_count} already had switchers")

if __name__ == '__main__':
    main()
