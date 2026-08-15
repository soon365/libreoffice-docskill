/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

function assertError(code, exceptionMessage)
{
    try
    {
        code();
    }
    catch (error)
    {
        if (error != exceptionMessage)
        {
            console.log("Exception message mismatch:\n" +
                        "Expected: " + exceptionMessage + "\n" +
                        "Actual:   " + error);
        }
        console.assert(error == exceptionMessage);
        return;
    }

    console.log("exception expected but none was thrown");
    console.assert(false);
}
