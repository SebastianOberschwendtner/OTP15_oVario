# Welcome!
You want contribute to the *joVario*? Awesome! :sunglasses:

There are some general rules to keep the code nice and tidy:
- [Design Principles](#design-principles)
  - [Scheduler](#scheduler)
  - [Arbiter](#arbiter)
  - [Inter Process Communciation](#inter-process-communication)
- [Naming Convention](#naming-convention)
  - [Defines](#defines)
  - [Defines for *Arbiter* Commands](#defines-for-arbiter-commands)
  - [Defines for *Arbiter* Sequences](#defines-for-arbiter-sequences)
  - [Variables](#variables)
  - [Functions](#functions)
- [Comments](#comments)
  - [File Header](#file-header)
  - [Function Header](#function-header)
  - [Header of *Arbiter* Command](#header-of-arbiter-command)
- [Commits](#commits)
- [Release Management](#release-management)
- [License](#license)

# Design Principles
The vario uses these design principles to handle the different tasks running on the vario. It is kind of an *operating system*, but not quite.
Still it is does the job and gives a nice performance.

&rArr; Refer to the [wiki](https://github.com/knuffel-v2/OTP15_oVario/wiki/home) for further information of the design principles.

## Scheduler
The scheduler is responsible for the coarse timing of all the tasks.

## Arbiter
The arbiter is the main concept for the processing of commands within tasks.

## Inter Process Communication
The Inter Process Communication, short **IPC**, is the main concept of sharing data and commands between the different tasks.

# Naming Convention
There are some simple rules on how to name things.

## Defines
General defines should be **all uppercase** with a **short** and **concise** name. Use **underscores** to make the name more readable.

Example:
```c
#define CONTEXT_NEW_SYMBOL  value // optional comment.
```

Try to include the context of the define in the beginning of the name.

## Defines for *Arbiter* Commands
The defines for the arbiter commands follow the same rulse as the normal [define](#defines), but have the following additional rules:
- **Always** include the context in the beginning of the name
- After the context **always** use the abbreviation *CMD*, telling that the define is a command.
- Use increasing numbers for each define within one task.
- Comment what call type the defined command is.

Example:
```c
#define CONTEXT_CMD_COMMAND_NAME  number // call-type
```

## Defines for *Arbiter* Sequences
The defines of the sequences for an arbiter command is the define for the commands itself, but instead of the *command abbreviation* you should insert
the keyword *SEQUENCE*.

## Variables
Variables are **all lowercase**. In the beginning they should have an abbreviation with the variable type. The sign is not part of this abbreviation.
Examples for the abbreviations of the variable type:
> The *p* stands for pointer.

|Type|Abbreviation|
|---:|:---:|
|char|`ch_`|
|*char|`pch_`|
|int|`i_`|
|*int|`pi_`|
|long|`l_`|
|*long|`pl_`|

You do not need to use these variable types, you can use the `uint8_t` type for example, this naming convention is just a recommendation.
> For more complex typedefs, the abbreviation does not make sense anymore.

Example:
```c
unsigned char pch_array = {}; //Example pointer to a char array
```

## Functions
Functions follow the same rules as [variables](#variables), but they have a short abbreviation of the context in the beginning of the name.
This is an attempt to mimic the name spaces of *C++*. The abbreviation should be as **short** as possible. The return type does not have to be in
the name.

# Comments
We use the *doxygen* documentation flags in the headers, to make them more readable. Although we do not export the documentation as *html*, the flags
are useful, because *VSCode* recognizes them, and renders them when hovering over the function name.

In general try to code like the person who is reading your code knows where you live. :wink:

## File Header
The file header gives a short description of what the file does. Also the [license](#license) information is added here.

Example:
```c
/**
 ******************************************************************************
 * @file    filename.c
 * @author  name
 * @version Vx.x
 * @date    dd-Month-yyyy
 * @brief   Description
 ******************************************************************************
 */
 ```
## Function Header
A function header gives a short **description**, defines the **inputs** and the **return value**. Addtitional **details** are optional:

```c
/**
 * @brief Descritpion
 * @param input1 Description of input, if any.
 * @param input2 Description of input, if any.
 * @return Description of th return value, if any.
 */
 ```
 Possible `@details` are:
 - *interrupt handler*
 - *inline function*
 
 You can also use additional comments:
 - `@todo`: Note what has to be done
 - `@deprecated`: Why this function is no longer used.
 
## Header of *Arbiter* Command
The header of an arbiter command is a bit different to distinguish them to normal functions:
```c
/**
 ***********************************************************
 * @brief Description
 ***********************************************************
 * 
 * @param input1 Description of input, if any.
 * @return Description of return value, if any.
 * @details call-type
 ***********************************************************
 */
 ```
 It follows similar rules than the [function header](#function-header), but the `@details` of the call type is **not** optional.
 
# Commits
- Commit messages should be clear and concise 
- Use **English** as far as possible.
- Use the # to close issues. 
- *Present* tense is **not** required. 

Symbols for commits:
- :bug: `:bug:` Bugfixes
- :racehorse: `:racehorse:` Performance Improvement
- :art: `:art:` Made Code More Readable
- :memo: `:memo:` Improved Documentation

# Release Management
*Tags* are used for the release management. The tag number is:
- **v***Major.Minor.Bugfix*
- Document every change in the [changelog](CHANGELOG.md)

> A release with release notes is only published on **minor** changes. Bugfixes are just pushed as tags.

# License

The following should be included in every file header:
> Adjust the name of the author according to who created the file.
```c
/**
 * joVario Firmware
 * Copyright (c) 2020 Sebastian Oberschwendtner, sebastian.oberschwendtner@gmail.com
 * Copyright (c) 2020 Jakob Karpfinger, kajacky@gmail.com
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
 ```
