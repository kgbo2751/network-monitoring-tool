from setuptools import setup, Extension
import pybind11

ext_modules = [
    Extension(
        "network_monitor",
        ["main.cpp"],
        include_dirs=[pybind11.get_include()],
        libraries=["iphlpapi", "ws2_32", "psapi"],
        language="c++"
    ),
]

setup(
    name="network_monitor",
    ext_modules=ext_modules,
)