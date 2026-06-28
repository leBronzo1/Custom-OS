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

def system_info():
    cpu = get_cpu_data()
    memory = get_memory_data()
    disk = get_disk_data()

    print(f"Current Time: {get_current_time()}")
    print(f"Boot Time: {get_boot_time()}")

    print(f"CPU Cores: {cpu['cores']}")
    print(f"CPU Usage: {cpu['usage']}%")

    print(f"Memory: {memory['percent']}%")
    print(f"Disk: {disk['percent']}%")

system_info()