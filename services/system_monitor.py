import json
import time
import psutil

def get_cpu_data():
    return {
        "cores": psutil.cpu_count(),
        "usage": psutil.cpu_percent(interval=1)
    }

def get_memory_data():
    memory = psutil.virtual_memory()
    return {
        "total": memory.total,
        "used": memory.used,
        "available": memory.available,
        "percent": memory.percent
    }

def get_disk_data():
    disk = psutil.disk_usage("/")
    return {
        "total": disk.total,
        "used": disk.used,
        "free": disk.free,
        "percent": disk.percent
    }

def get_boot_time():
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