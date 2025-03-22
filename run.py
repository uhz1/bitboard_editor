import os
import subprocess

def get_all_c_file_paths_helper(base_dir: str, cur_dir: str, files: list[str]) -> list[str]:
  for file in os.listdir(cur_dir):
    path = os.path.join(cur_dir, file)
    if os.path.isfile(path) and os.path.splitext(path)[1] == ".c":
      file = os.path.relpath(path, base_dir)
      files.append(file)
    elif os.path.isdir(path):
      get_all_c_file_paths_helper(base_dir, path, files)
  return files

def get_all_c_file_paths(base_dir: str) -> list[str]:
  files: list[str] = []
  return get_all_c_file_paths_helper(base_dir, base_dir, files)

def main():
  exec_name = "main"
  files: str = ' '.join(get_all_c_file_paths(os.getcwd()))
  res = subprocess.run(f"clang -o {exec_name} {files} -lSDL2_ttf $(sdl2-config --cflags --libs)", shell=True)
  if res.returncode == 0:
    print(f"Compiled {files} -> {exec_name}")
  else:
    print("Dramatic error compiling")

  subprocess.run(f"leaks --atExit -- ./{exec_name}", shell=True)

if __name__ == "__main__":
  main()
