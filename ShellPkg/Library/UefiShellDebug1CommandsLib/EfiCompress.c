/** @file
  Main file for EfiCompress shell Debug1 function.

  Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include "Compress.h"

SHELL_STATUS
EFIAPI
ShellCommandRunEfiCompress (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  SHELL_FILE_HANDLE   InShellFileHandle;
  SHELL_FILE_HANDLE   OutShellFileHandle;
  UINT64              OutSize;
  VOID                *OutBuffer;
  UINT64              InSize;
  VOID                *InBuffer;
  CHAR16              *InFileName;
  CONST CHAR16        *OutFileName;

  InFileName          = NULL;
  OutFileName         = NULL;
  OutSize             = 0;
  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;
  OutBuffer           = NULL;
  InShellFileHandle   = NULL;
  OutShellFileHandle  = NULL;
  InBuffer            = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount(Package) < 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      InFileName = ShellFindFilePath(ShellCommandLineGetRawValue(Package, 1));
      OutFileName = ShellCommandLineGetRawValue(Package, 2);
      Status = ShellOpenFileByName(InFileName, &InShellFileHandle, EFI_FILE_MODE_READ, 0);
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_OPEN_FAIL), gShellDebug1HiiHandle, ShellCommandLineGetRawValue(Package, 1), Status);
        ShellStatus = SHELL_NOT_FOUND;
      }
      Status = ShellOpenFileByName(OutFileName, &OutShellFileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_OPEN_FAIL), gShellDebug1HiiHandle, ShellCommandLineGetRawValue(Package, 2), Status);
        ShellStatus = SHELL_NOT_FOUND;
      }
      if (FileHandleIsDirectory(InShellFileHandle) == EFI_SUCCESS){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_NOT_DIR), gShellDebug1HiiHandle, InFileName);
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
      if (FileHandleIsDirectory(OutShellFileHandle) == EFI_SUCCESS){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_NOT_DIR), gShellDebug1HiiHandle, OutFileName);
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
      Status = gEfiShellProtocol->GetFileSize(InShellFileHandle, &InSize);
      ASSERT_EFI_ERROR(Status);
      InBuffer = AllocateZeroPool((UINTN)InSize);
      ASSERT(InBuffer != NULL);
      Status = gEfiShellProtocol->ReadFile(InShellFileHandle, &((UINTN)InSize), InBuffer);
      ASSERT_EFI_ERROR(Status);
      Status = Compress(InBuffer, InSize, OutBuffer, &OutSize);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        OutBuffer = AllocateZeroPool((UINTN)OutSize);
        ASSERT(OutBuffer != NULL);
        Status = Compress(InBuffer, InSize, OutBuffer, &OutSize);
      }
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_EFI_COMPRESS_FAIL), gShellDebug1HiiHandle, Status);
        ShellStatus = SHELL_DEVICE_ERROR;
      } else {
        Status = gEfiShellProtocol->WriteFile(OutShellFileHandle, &((UINTN)OutSize), OutBuffer);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_WRITE_FAIL), gShellDebug1HiiHandle, OutFileName, Status);
          ShellStatus = SHELL_DEVICE_ERROR;
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
  }
  if (InFileName != NULL) {
    FreePool(InFileName);
  }
  if (InShellFileHandle != NULL) {
    gEfiShellProtocol->CloseFile(InShellFileHandle);
  }
  if (OutShellFileHandle != NULL) {
    gEfiShellProtocol->CloseFile(OutShellFileHandle);
  }
  if (InBuffer != NULL) {
    FreePool(InBuffer);
  }
  if (OutBuffer != NULL) {
    FreePool(OutBuffer);
  }

  return (ShellStatus);
}
