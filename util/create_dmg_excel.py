import xlsxwriter
from xlsxwriter.utility import xl_rowcol_to_cell
import json

MAX_LEVEL = 101

book = xlsxwriter.Workbook("vrx_damage.xlsx")
sheet = book.add_worksheet("damage")

with open("dmgdata.json") as f:
    dmgdata = json.loads(f.read())

for level in range(0, MAX_LEVEL):
    
    if level == 0:
        sheet.write(0, 0, "level")
        sheet.write(1, 0, "dmg_start")
        sheet.write(2, 0, "dmg_addon")
        continue

    sheet.write(level + 2, 0, level)

col = 1
for key in dmgdata:
    item = dmgdata[key]
    sheet.write(0, col, key)
    sheet.write_number(1, col, item["damage_start"])
    sheet.write_number(2, col, item["damage_addon"])

    for level in range(1, MAX_LEVEL):
        cell_dmg_base = xl_rowcol_to_cell(1, col)
        cell_level = xl_rowcol_to_cell(2 + level, 0)
        cell_dmg_addon = xl_rowcol_to_cell(2, col)

        cellstr = "={} + {} * {}".format(
                    cell_dmg_base,
                    cell_level,
                    cell_dmg_addon
            )
        sheet.write(
                2 + level, col, 
                cellstr
        )

    col += 1
    
book.close()