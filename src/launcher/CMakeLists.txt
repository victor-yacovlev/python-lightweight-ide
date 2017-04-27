project(kumir2-python)
cmake_minimum_required(VERSION 3.0)

find_package(Kumir2 REQUIRED)

kumir2_add_launcher(
    NAME            kumir2-python
    SPLASHSCREEN    "splashscreen-python.png"
    WINDOW_ICON     "kumir2-icon-python.png"
    CONFIGURATION   "CourseManager,Editor,Python3Language,!CoreGUI\(icon=python,nosessions\)"
)
