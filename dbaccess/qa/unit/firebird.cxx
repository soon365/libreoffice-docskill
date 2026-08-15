/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dbtest_base.cxx"

#include <com/sun/star/sdb/XOfficeDatabaseDocument.hpp>
#include <com/sun/star/sdbc/XColumnLocate.hpp>
#include <com/sun/star/sdbc/XConnection.hpp>
#include <com/sun/star/sdbc/XParameters.hpp>
#include <com/sun/star/sdbc/XPreparedStatement.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XStatement.hpp>

using namespace ::com::sun::star;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::uno;

class FirebirdTest
    : public DBTestBase
{
public:
    void testEmptyDBConnection();
    void testIntegerDatabase();
    void testTdf132924();
    void testTdf153057();

    CPPUNIT_TEST_SUITE(FirebirdTest);
    CPPUNIT_TEST(testEmptyDBConnection);
    CPPUNIT_TEST(testIntegerDatabase);
    CPPUNIT_TEST(testTdf132924);
    CPPUNIT_TEST(testTdf153057);
    CPPUNIT_TEST_SUITE_END();
};

/**
 * Test the loading of an "empty" file, i.e. the embedded database has not yet
 * been initialised (as occurs when a new .odb is created and opened by base).
 */
void FirebirdTest::testEmptyDBConnection()
{
    createTempCopy(u"firebird_empty.odb");
    uno::Reference< XOfficeDatabaseDocument > xDocument =
        getDocumentForUrl(maTempFile.GetURL());

    getConnectionForDocument(xDocument);
}

/**
 * Test reading of integers from a known .odb to verify that the data
 * can still be read on all systems.
 */
void FirebirdTest::testIntegerDatabase()
{
    loadFromFile(u"firebird_integer_ods12.odb");
    uno::Reference< XOfficeDatabaseDocument > xDocument(mxComponent, UNO_QUERY_THROW);

    uno::Reference< XConnection > xConnection =
        getConnectionForDocument(xDocument);

    uno::Reference< XStatement > xStatement = xConnection->createStatement();
    CPPUNIT_ASSERT(xStatement.is());

    uno::Reference< XResultSet > xResultSet = xStatement->executeQuery(
        u"SELECT * FROM TESTTABLE"_ustr);
    CPPUNIT_ASSERT(xResultSet.is());
    CPPUNIT_ASSERT(xResultSet->next());

    uno::Reference< XRow > xRow(xResultSet, UNO_QUERY);
    CPPUNIT_ASSERT(xRow.is());
    uno::Reference< XColumnLocate > xColumnLocate(xRow, UNO_QUERY);
    CPPUNIT_ASSERT(xColumnLocate.is());

    CPPUNIT_ASSERT_EQUAL(sal_Int16(-30000),
        xRow->getShort(xColumnLocate->findColumn(u"_SMALLINT"_ustr)));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(-2100000000),
        xRow->getInt(xColumnLocate->findColumn(u"_INT"_ustr)));
    CPPUNIT_ASSERT_EQUAL(SAL_CONST_INT64(-9000000000000000000),
        xRow->getLong(xColumnLocate->findColumn(u"_BIGINT"_ustr)));
    CPPUNIT_ASSERT_EQUAL(u"5"_ustr,
        xRow->getString(xColumnLocate->findColumn(u"_CHAR"_ustr)));
    CPPUNIT_ASSERT_EQUAL(u"5"_ustr,
        xRow->getString(xColumnLocate->findColumn(u"_VARCHAR"_ustr)));

    CPPUNIT_ASSERT(!xResultSet->next()); // Should only be one row
}

void FirebirdTest::testTdf132924()
{
    loadFromFile(u"tdf132924.odb");
    uno::Reference< XOfficeDatabaseDocument > xDocument(mxComponent, UNO_QUERY_THROW);
    uno::Reference<XConnection> xConnection = getConnectionForDocument(xDocument);

    uno::Reference<XStatement> xStatement = xConnection->createStatement();
    CPPUNIT_ASSERT(xStatement.is());

    uno::Reference<XResultSet> xResultSet = xStatement->executeQuery(u"SELECT * FROM AliasTest"_ustr);
    CPPUNIT_ASSERT(xResultSet.is());
    CPPUNIT_ASSERT(xResultSet->next());

    uno::Reference<XRow> xRow(xResultSet, UNO_QUERY);
    CPPUNIT_ASSERT(xRow.is());
    uno::Reference<XColumnLocate> xColumnLocate(xRow, UNO_QUERY);
    CPPUNIT_ASSERT(xColumnLocate.is());

    // Without the fix in place, this test would have failed with:
    // - Expected: 1
    // - Actual  : The column name 'TestId' is not valid
    CPPUNIT_ASSERT_EQUAL(sal_Int16(1), xRow->getShort(xColumnLocate->findColumn(u"TestId"_ustr)));
    CPPUNIT_ASSERT_EQUAL(u"TestName"_ustr, xRow->getString(xColumnLocate->findColumn(u"TestName"_ustr)));
}

/**
 * Test for tdf#153057: filtering on DATE, TIME and TIMESTAMP columns via
 * a string value.  Before the fix, OPreparedStatement::setString() had no
 * cases for DataType::DATE / TIME / TIMESTAMP and would throw
 * "Incorrect type for setString" when a filter string was applied to such
 * a column in Base.
 */
void FirebirdTest::testTdf153057()
{
    // Create a fresh embedded Firebird database in a temp file.
    createTempCopy(u"firebird_empty.odb");
    uno::Reference<XOfficeDatabaseDocument> xDocument = getDocumentForUrl(maTempFile.GetURL());

    uno::Reference<XConnection> xConnection = getConnectionForDocument(xDocument);
    CPPUNIT_ASSERT(xConnection.is());

    // Create a table with DATE, TIME and TIMESTAMP columns and two rows.
    uno::Reference<XStatement> xStmt = xConnection->createStatement();
    xStmt->execute(u"CREATE TABLE \"TEMPORAL\" ("
                   " \"ID\"   INTEGER NOT NULL PRIMARY KEY,"
                   " \"D\"    DATE,"
                   " \"T\"    TIME,"
                   " \"TS\"   TIMESTAMP)"_ustr);

    xStmt->execute(
        u"INSERT INTO \"TEMPORAL\" VALUES(1, '2024-07-15', '14:30:00', '2024-07-15 14:30:00')"_ustr);
    xStmt->execute(
        u"INSERT INTO \"TEMPORAL\" VALUES(2, '2025-01-01', '09:00:00', '2025-01-01 09:00:00')"_ustr);
    xConnection->commit();

    // --- Test DATE filtering via setString ---
    // Before the fix this would throw "Incorrect type for setString".
    {
        uno::Reference<XPreparedStatement> xPS = xConnection->prepareStatement(
            u"SELECT \"ID\" FROM \"TEMPORAL\" WHERE \"D\" = ?"_ustr);
        uno::Reference<XParameters> xParams(xPS, UNO_QUERY_THROW);
        xParams->setString(1, u"2024-07-15"_ustr);

        uno::Reference<XResultSet> xRS = xPS->executeQuery();
        CPPUNIT_ASSERT_MESSAGE("DATE filter: expected at least one row", xRS->next());
        uno::Reference<XRow> xRow(xRS, UNO_QUERY_THROW);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("DATE filter: wrong row returned", sal_Int32(1),
                                     xRow->getInt(1));
        CPPUNIT_ASSERT_MESSAGE("DATE filter: expected exactly one row", !xRS->next());
    }

    // --- Test TIME filtering via setString ---
    {
        uno::Reference<XPreparedStatement> xPS = xConnection->prepareStatement(
            u"SELECT \"ID\" FROM \"TEMPORAL\" WHERE \"T\" = ?"_ustr);
        uno::Reference<XParameters> xParams(xPS, UNO_QUERY_THROW);
        xParams->setString(1, u"09:00:00"_ustr);

        uno::Reference<XResultSet> xRS = xPS->executeQuery();
        CPPUNIT_ASSERT_MESSAGE("TIME filter: expected at least one row", xRS->next());
        uno::Reference<XRow> xRow(xRS, UNO_QUERY_THROW);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("TIME filter: wrong row returned", sal_Int32(2),
                                     xRow->getInt(1));
        CPPUNIT_ASSERT_MESSAGE("TIME filter: expected exactly one row", !xRS->next());
    }

    // --- Test TIMESTAMP filtering via setString ---
    {
        uno::Reference<XPreparedStatement> xPS = xConnection->prepareStatement(
            u"SELECT \"ID\" FROM \"TEMPORAL\" WHERE \"TS\" = ?"_ustr);
        uno::Reference<XParameters> xParams(xPS, UNO_QUERY_THROW);
        xParams->setString(1, u"2025-01-01 09:00:00"_ustr);

        uno::Reference<XResultSet> xRS = xPS->executeQuery();
        CPPUNIT_ASSERT_MESSAGE("TIMESTAMP filter: expected at least one row", xRS->next());
        uno::Reference<XRow> xRow(xRS, UNO_QUERY_THROW);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("TIMESTAMP filter: wrong row returned", sal_Int32(2),
                                     xRow->getInt(1));
        CPPUNIT_ASSERT_MESSAGE("TIMESTAMP filter: expected exactly one row", !xRS->next());
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(FirebirdTest);

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
