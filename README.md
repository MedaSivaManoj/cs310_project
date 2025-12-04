# Custom Multi-Profile Linux Shell
## 📌 Project Overview
This project implements a **custom Linux shell** written in C. It provides the functionality of a normal Linux terminal while also supporting **five unique user profiles**, each containing its own built-in commands and environment behavior. The shell supports executing Linux system commands, advanced operators (`&&`, pipes), persistent history storage, directory creation, and color-coded prompts. The aim is to offer a more interactive terminal experience than a typical CLI.

## 🔥 Key Capabilities
- Execute normal Linux commands using `fork()` and `execvp()`
- **Five profile-based work modes**: Core, Ops, Data, Net, Sec
- Built-in commands that vary by profile
- Custom colored prompt based on profile
- `cd` implementation (without launching a separate process)
- Persistent **command history** saved across sessions
- Supports `cmd1 | cmd2` pipe processing
- Supports conditional execution `cmd1 && cmd2`
- Auto-generates folders required for built-in commands on first run

## 🧠 Profiles & Built-in Commands Table
| Profile | Prompt | Built-in Commands |
|--------|--------|------------------|
| **Core** | `Core>` | `sanitize`, `backup`, `unhide` |
| **Ops** | `Ops>` | `truncate_important`, `generate_corrupt`, `hide_main` |
| **Data** | `Data>` | `mkdata`, `motivate`, `tips` |
| **Net** | `Net>` | `netquote`, `netquiz`, `find_target` |
| **Sec** | `Sec>` | `scan_temp`, `secure_backup`, `clean_temp` |

### Built-in commands available for every profile
```
cd
history
help
exit
```

## 🗂 Auto-Created Directory Structure
On first execution, the shell creates:
```
backup_good_files/
corrupted_files/
good_files/
hidden/
main/
target.txt
target_location.txt
```
These directories are used by profile-specific commands to perform file actions safely instead of modifying user system files.

## ⚙ Requirements
Install GNU Readline library before compiling:
```bash
sudo apt install libreadline-dev
```

## 🧪 Compilation
Inside the project directory:
```bash
gcc custom_shell.c -lreadline -o custom_shell
```

## ▶️ Running the Shell
```
./custom_shell
```

## 🧭 Profile Selection Wizard — How to Choose a Profile
On startup, the shell displays five yes/no questions. Based on the answers, the shell selects a profile.

| To enter | Say **yes** only on |
|---------|---------------------|
| Core | Q1 |
| Ops | Q2 |
| Data | Q3 |
| Net | Q4 |
| Sec | Q5 |

Example (to enter Net profile):
```
Q1: no
Q2: no
Q3: no
Q4: yes
Q5: no
```

## 🎨 Prompt Styles
Prompt changes depending on the selected profile:
```
Core>    Ops>    Data>    Net>    Sec>
```

## 🧾 Example Complete Output Session
```
$ ./custom_shell

Profile selection wizard (answer yes/no):
Q1: Do you like cleaning, organizing and maintaining systems? no
Q2: Do you enjoy automation, deployment and operations? yes
Q3: Do you like working with data and logs? no
Q4: Are you interested in networking and connectivity? no
Q5: Are you interested in security and monitoring? no

Selected profile: Ops
Profile activated successfully.

Ops> ls
backup_good_files  corrupted_files  good_files  hidden  main  target.txt  target_location.txt

Ops> generate_corrupt
[Ops] 5 corrupted files created in corrupted_files/

Ops> ls corrupted_files
file1.txt  file2.txt  file3.txt  file4.txt  file5.txt

Ops> history
1: ls
2: generate_corrupt
3: ls corrupted_files
4: history

Ops> cd hidden
Ops> pwd
/home/user/project/hidden

Ops> ls | wc
      3      3     42

Ops> ls missing.txt && echo worked
ls: cannot access 'missing.txt': No such file or directory
# (Second command does NOT execute because first failed)

Ops> exit
Exiting shell... Goodbye!
```

## 🔍 Explanation of Advanced Features
| Feature | Description |
|--------|-------------|
| `command1 && command2` | Runs command2 only if command1 succeeds (return status 0) |
| `command1 | command2` | Pipes output of command1 to command2 |
| Persistent history | Every executed command is appended to `.custom_shell_history` |
| `cd <path>` | Changes working directory **without creating a child process** |

## ❗ Error Handling Messages
| Situation | Response Example |
|----------|------------------|
| Unknown built-in | `Unknown command. Type 'help' to see available commands.` |
| Invalid Linux command | `Could not execute command.` |
| Failed pipe execution | `Pipe could not be initialized` |
| Wrong help argument | `Unknown help topic. Try: help` |



## 🔚 Conclusion
This custom Linux shell extends traditional terminal functionality by introducing **profile-based environments, automated file operations, persistent command history, pipe and conditional execution operators, and customizable prompts**. These enhancements make the shell more interactive, user-oriented, and practical compared to a basic CLI.
