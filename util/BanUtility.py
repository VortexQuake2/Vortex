#  This program will allow the user to modify a Vortex ban list
#  (listip.cfg), and allow them to ban the player for a predetermined
#  amount of time. Should be run from within the /vortex subfolder.

import threading
import re
from datetime import datetime


def append_ban_list(IPAddress, BanType, BanLength):
    # Opens ban list 'listip' in append mode, appends ban type,
    # ban length, and date and time of ban.

    try:
        banlist = open("listip.cfg", mode='a', encoding='utf-8')
        # file operations
        if BanType == 'mute':
            banlist.write("\nsv addip " + IPAddress + " silence " + "// Ban Length: " +
                          BanLength + " minutes " + str(datetime.now()))
        elif BanType == 'ban':
            banlist.write("\nsv addip " + IPAddress + " // Ban Length: " +
                          BanLength + " minutes " + str(datetime.now()))
    finally:
        banlist.close()


def remove_from_ban_list(IPAddress):
    try:
        banlist = open("listip.cfg", mode='r', encoding='utf-8') # read
        lines = banlist.readlines()
        banlist = open("listip.cfg", mode='w', encoding='utf-8') # write
        for line in lines:
            if not re.findall(IPAddress, line):
                banlist.write(line)

    finally:
        banlist.close()


def view_ban_list():
    try:
        banlist = open("listip.cfg", mode='r', encoding='utf-8')
        for line in banlist:
            print(line[9:])
    finally:
        banlist.close()


def ban_menu():

    print("Enter IP Address to ban: ")
    IPAddress = input()
    print("Enter ban type: (mute) for mute, (ban) for ban")
    BanType = input()
    print("Enter ban duration in minutes: (1440 = 1 day, 10080 = 1 week. 0 = permanent)")
    Duration = input()

    TickerMinutes = int(Duration) * 60  # Duration in minutes * 60 seconds
    append_ban_list(IPAddress, BanType, Duration)
    if TickerMinutes != "0":
        ticker = threading.Event()
        while not ticker.wait(TickerMinutes):
            remove_from_ban_list(IPAddress)
            return


def unban_menu():
    print("Enter an IP address to unban: ")
    IPAddress = input()
    remove_from_ban_list(IPAddress)


print("Vortex Ban Utility")
print("Would you like to (ban), (unban), (view) the ban list, or (exit)?")
menuChoice = input()
if menuChoice == "ban" or menuChoice == "b":
    ban_menu()
elif menuChoice == "unban" or menuChoice == 'u':
    unban_menu()
elif menuChoice == "view" or menuChoice == 'v':
    view_ban_list()
