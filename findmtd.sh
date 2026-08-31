#!/bin/sh

# 1. Check if an argument was provided
if [ -z "$1" ]; then
    echo "Usage: $0 <target_volume_label>" >&2
    echo "Example: $0 jffs2" >&2
    exit 1
fi

TARGET_LABEL="$1"
MTD_NUM=""

# 2. Iterate through active UBI volumes in sysfs
for vol in /sys/class/ubi/ubi*_*/; do
    if [ -f "${vol}name" ]; then
        CURRENT_LABEL=$(cat "${vol}name")
        if [ "$CURRENT_LABEL" = "$TARGET_LABEL" ]; then
            # Found the volume! Extract the parent MTD number
            MTD_NUM=$(cat "${vol}device/mtd_num")
            break
        fi
    fi
done

# 3. Output the result or throw an error if not found
if [ -n "$MTD_NUM" ]; then
    echo "Volume '$TARGET_LABEL' is tied to /dev/mtd$MTD_NUM"
else
    echo "Error: UBI volume with label '$TARGET_LABEL' not found." >&2
    exit 1
fi

