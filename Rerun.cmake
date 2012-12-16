set(FailedFileName FailedTests.log)
if(EXISTS "Testing/Temporary/LastTestsFailed.log")
    file(STRINGS "Testing/Temporary/LastTestsFailed.log" FailedTests)
    string(REGEX REPLACE "([0-9]+):[^;]*" "\\1" FailedTests "${FailedTests}")
    list(SORT FailedTests)
    list(GET FailedTests 0 FirstTest)
    set(FailedTests "${FirstTest};${FirstTest};;${FailedTests};")
    string(REPLACE ";" "," FailedTests "${FailedTests}")
    file(WRITE ${FailedFileName} ${FailedTests})
else()
    file(WRITE ${FailedFileName} "")
endif()
