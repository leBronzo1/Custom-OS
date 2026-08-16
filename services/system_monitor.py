import json
import os
import platform
import subprocess
import time
try:
    import psutil
except ImportError:
    psutil = None

def get_cpu_data():
    if psutil is None:
        return {"cores": os.cpu_count() or 0, "usage": 0.0}
    return {
        "cores": psutil.cpu_count(),
        "usage": psutil.cpu_percent(interval=None)
    }

def get_memory_data():
    if psutil is None:
        if platform.system() == "Darwin":
            try:
                total = int(subprocess.check_output(
                    ["sysctl", "-n", "hw.memsize"], stderr=subprocess.DEVNULL
                ))
                vm_stat = subprocess.check_output(["vm_stat"], text=True)
                page_size = os.sysconf("SC_PAGE_SIZE")
                pages = {}
                for line in vm_stat.splitlines()[1:]:
                    key, _, value = line.partition(":")
                    if value:
                        pages[key.strip()] = int(value.strip().rstrip("."))
                available = page_size * (pages.get("Pages free", 0) + pages.get("Pages speculative", 0))
            except (OSError, subprocess.SubprocessError, ValueError):
                total = os.sysconf("SC_PAGE_SIZE") * os.sysconf("SC_PHYS_PAGES")
                available = total
        else:
            values = {}
            with open("/proc/meminfo", encoding="utf-8") as meminfo:
                for line in meminfo:
                    key, value = line.split(":", 1)
                    values[key] = int(value.split()[0]) * 1024
            total = values["MemTotal"]
            available = values.get("MemAvailable", values.get("MemFree", 0))
        used = total - available
        return {"total": total, "used": used, "available": available,
                "percent": (used / total * 100) if total else 0.0}
    memory = psutil.virtual_memory()
    return {
        "total": memory.total,
        "used": memory.used,
        "available": memory.available,
        "percent": memory.percent
    }

def get_disk_data():
    if psutil is None:
        stat = os.statvfs("/")
        total = stat.f_blocks * stat.f_frsize
        free = stat.f_bavail * stat.f_frsize
        used = total - free
        return {"total": total, "used": used, "free": free,
                "percent": (used / total * 100) if total else 0.0}
    disk = psutil.disk_usage("/")
    return {
        "total": disk.total,
        "used": disk.used,
        "free": disk.free,
        "percent": disk.percent
    }

def get_boot_time():
    if psutil is None:
        return "Unavailable"
    return time.strftime(
        "%Y-%m-%d %H:%M:%S",
        time.localtime(psutil.boot_time())
    )

def get_current_time():
    return time.strftime("%Y-%m-%d %H:%M:%S")

def system_info_json():
    """Return all system information as a JSON string."""
    data = {
        "current_time": get_current_time(),
        "boot_time": get_boot_time(),
        "cpu": get_cpu_data(),
        "memory": get_memory_data(),
        "disk": get_disk_data()
    }

    return json.dumps(data, indent=4)

if __name__ == "__main__":
    print(system_info_json())
