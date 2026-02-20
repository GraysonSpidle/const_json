// This file contains tests for the const_json library
#include "const_json.h"

#include <iostream>
#include <typeinfo>
#include <string>

int main() {
	std::string stuff = R"---({ "test" : [1.0, 123131,"2346",3.0,],
"another": false, "tryingToBreakIt": null, "onemore": { "innerObject":{"id":1}, "innerArray": [1,2,3,4]}})---";

	SPIDLE_CONST_JSON_USING;

	using ComplicatedArray = Array<"test", Int_, String_, Double_, Int<"thisWillBeRemoved">>;
	using InnerObject = Object<"innerObject", Int<"id">>;
	using InnerArray = Array<"innerArray", Int_>;
	using ComplicatedObject = Object<"onemore", InnerObject, InnerArray>;
	using JsonSchema = Object_<
		ComplicatedArray,
		Variant<"another", Bool<"bool">>,
		ComplicatedObject,
		Optional<Double<"willNotBeThere">>
	>;

	std::istringstream iss{ stuff };
	typename JsonSchema::rettype v2;
	try {
		v2 = const_json::parse<JsonSchema>(iss);
	}
	catch (const_json::err::NotAllMembersPresentError& e) {
		std::cout << "Not all members are present!" << std::endl;
		std::cout << e.path << std::endl;
		for (auto it = e.membersAbsent.begin(); it != e.membersAbsent.end(); ++it) {
			std::cout << '\t' << *it << std::endl;
		}
		return 1;
	}
	catch (const_json::err::BadTypeError& e) {
		std::cout << "Unexpected type for " << e.path << std::endl;
		std::cout << "Expected: " << e.expected.get().name() << " (you might have to unmangle this)" << std::endl;
		return 1;
	}
	catch (const_json::err::MalformedInputError) {
		std::cout << "Malformed Input Error at char: " << static_cast<size_t>(iss.tellg()) << std::endl;
		return 1;
	}

	// Example of doing something with an array member in the result

	auto testArray = const_json::getMember<JsonSchema, "test">(v2);
	for (auto it = testArray.begin(); it != testArray.end(); ++it) {
		if (std::holds_alternative<intmax_t>(*it)) {
			std::cout << std::get<intmax_t>(*it) << std::endl;
		}
		else if (std::holds_alternative<double>(*it)) {
			std::cout << std::fixed << std::get<double>(*it) << std::endl;
		}
		else if (std::holds_alternative<std::string>(*it)) {
			std::cout << std::get<std::string>(*it) << std::endl;
		}
		else // This won't happen
			continue;
	}

	// Example of doing something with an object member in the result

	auto& id = const_json::getMember<JsonSchema, "onemore", "innerObject", "id">(v2);
	id = 1234;
	auto& innerArray = const_json::getMember<JsonSchema, "onemore", "innerArray">(v2);
	static_assert(std::same_as<Int_::rettype, typename const_json::get_member_schema_t<JsonSchema, "onemore", "innerArray">::element_schema::rettype>);
	for (auto it = innerArray.begin(); it != innerArray.end(); ++it) {
		*it = 5;
	}

	// Example of using named alternatives in Variant

	auto& b = const_json::getMember<JsonSchema, "another", "bool">(v2);
	b = !b;

	// Writing it all to a string

	constexpr const_json::Formatting fmt = const_json::Formatting {
		const_json::Formatting::IndentStyle::Spaces,
		const_json::Formatting::IndentSize{4}
	};

	std::ostringstream oss;
	oss << std::fixed;

	const_json::dump<JsonSchema, fmt>(v2, oss);
	
	std::cout << oss.str() << std::endl;

	std::cout << "\n\n" << std::endl;

// ==========================================================================================================================================================

	using EmployeePersonVehicleDetailsSchema = Object_<String<"vehicleMake">, String<"vehicleModel">, String<"vehicleYear">, String<"vehicleAddDate">, String<"vehicleRemoveDate">, String<"vehicleRegNo">>;
	using EmployeePersonAutoInsurancePolicySchema = Object_<String<"autoInsurancePolicyId">, String<"autoInsuranceCarrierName">, String<"autoInsuranceHolderName">, String<"autoInsurancePolicyStartDate">, String<"autoInsurancePolicyExpDate">>;
	using EmployeePersonEmergencyContactSchema = Object_<String<"contactName">, String<"contactInfo">, String<"contactRelationship">, String<"contactType">>;
	using EmployeeEmergencyContactArraySchema = Array<"emergencyContact", EmployeePersonEmergencyContactSchema>;
	using EmployeePersonSchema = Object<"person", String<"ethnicCode">, String<"birthDate">, String<"driverLicenseId">, String<"cobraSSN">, Array<"employerIds", String_>, Bool<"cobraOnly">, String<"geoCode">, String<"maritalStatus">, String<"marriedDate">, String<"smoker">, String<"peoStartDate">, String<"handicapped">, String<"blind">, String<"veteran">, String<"vietnamVet">, String<"disabledVet">, String<"hrpCitizenshipCountry">, Bool<"deceased">, String<"gender">, String<"taxZipCode">, Bool<"highlyCompensated">, Bool<"keyEmployee">, String<"overrideGeoCode">, String<"overrideGeoEndDate">, String<"nickname">, String<"deceasedDate">, String<"driverState">, String<"driverLicenseExpireDate">, String<"driverLicenseClass">, Bool<"ohioFormC112">, Bool<"unincorporatedResidence">, String<"w2AddressLine1">, String<"w2AddressLine2">, String<"w2City">, String<"w2State">, String<"w2PostalCode">, Bool<"w2ElecForm">, String<"w2ElecFormDate">, String<"legacyEmployeeId">, Bool<"hispanic">, Bool<"agricultureWorker">, String<"otherProtectedVet">, String<"activeDutyBadgeVet">, String<"recentlySeparatedVet">, String<"serviceMedalVet">, String<"preferredLanguage">, EmployeeEmergencyContactArraySchema, String<"userId">, Array<"autoPolicies", EmployeePersonAutoInsurancePolicySchema>, Array<"vehicleDetails", EmployeePersonVehicleDetailsSchema>, String<"personChecksum">>;
	using EmployeeEmployeeCompensationSchema = Object<"compensation", String<"ssn">, String<"payGroup">, String<"payPeriod">, Array<"payPeriodInfo", Object_<String<"periodCode">, String<"periodBase">>>, String<"defaultHours">, String<"payMethod">, String<"allocationTemplateId">, Array<"payAllocation", Object_<String<"locationCode">, String<"percent">, String<"job">, String<"deptCode">, String<"project">, String<"division">>>, String<"standardHours">, Array<"stateTax", Object_<String<"stateCode">, String<"nonResCertFiled">, String<"filingStatus">, String<"primaryAllowance">, String<"secondaryAllowance">, String<"exemptAmount">, String<"supplExemptAmount">, String<"overrideType">, String<"overrideAmount">, String<"reduceAddlWithholding">, String<"fixedWithholding">, String<"alternateCalc">, String<"stateW4Filed">, String<"stateW4Year">, Bool<"multipleJobs">, Int<"dependents">, Int<"otherIncome">, Int<"deductions">, String<"nonResCertYear">>>, String<"hourlyPay">, String<"salary">, String<"workShift">, String<"paidThruDate">, String<"eicFileStatus">, String<"fedFilingStatus">, String<"fedWithholdAllowance">, String<"fedOverrideType">, String<"fedOverrideAmt">, String<"fedTaxCalc">, Bool<"fedMultipleJobs">, Int<"fedDependents">, Int<"fedOtherIncome">, Int<"fedDeductions">, String<"hourlyPayRate">, String<"hourlyPayPeriod">, Bool<"deliverCheckHome">, Bool<"autoAcceptTimeSheet">, String<"longPayTable">, String<"longBasisDate">, String<"annualPay">, String<"effectiveDate">, Array<"localTax", Object_<String<"authId">, String<"filingStatus">, String<"primaryAllowance">, String<"nonResCert">, String<"addlWithheld">>>, String<"lastChangePct">, String<"lastChangeAmt">, String<"lastPayDate">, String<"quartile">, String<"compaRatio">, String<"benefitsBasePay">, String<"perDiemPay">, Bool<"taxCreditEmp">, String<"autoPayCode">, Array<"altPayRate", String_>, String<"pensionStatus">, String<"profitSharing">, Array<"eeImage", Object_<String<"masterId">, String<"verifyDateTime">, String<"verifyUserId">>>, Bool<"wagePlanCA">, String<"perfReviewLastDate">, String<"perfReviewNextDate">, Array<"i9imageAudit", Object_<String<"auditDate">, String<"auditUser">, String<"auditId">, String<"auditStep">>>, String<"perfReviewLastRating">, String<"compReviewNextDate">, String<"perfReviewLastTitle">, String<"fedW5Year">, Bool<"fedW5Filed">, Bool<"fedW4Filed">, String<"fedW4Year">, Array<"lockIns", Object_<String<"state">, String<"stateDate">, String<"authUser">>>, Bool<"formI9Filed">, String<"formI9RenewDate">, String<"notes">, Bool<"ficaExempt">, String<"benefitsPerHour">, Bool<"perfAgreement">, String<"payRate2">, String<"payRate3">, String<"payRate4">, String<"payChangeLastDate">, String<"overrideWorkGeocode">, String<"familyMemberMI">, String<"probationCodeMO">, Bool<"electronicPayStub">, Bool<"healthInsVT">, String<"lastWorkedDate">, String<"irsLockInDate">, Bool<"nonResAlien">, String<"alienRegExpDate">, String<"benefitSalary">, String<"citizenshipStatus">, String<"firstPayPeriodHours">, String<"compensationChecksum">, String<"providerNotifiedOn">, String<"alternateId">, Bool<"w8Filed">>;
	using EmployeeEmployeeDirectDepositSchema = Object<"directDeposit", String<"achStatus">, String<"voucherType">, Bool<"suppressAccountPrint">, Array<"achVoucher", Object_<String<"transitNum">, String<"accountNum">, String<"accountType">, String<"method">, Int<"amount">, Int<"limit">, String<"status">, String<"bankName">, String<"voucherTypeOverride">>>, String<"directDepositChecksum">>;
	using EmployeeEmployeeHsaSchema = Object<"hsa", Array<"hsaDepositSettings", Object_<String<"transitNum">, String<"accountNum">, String<"accountType">, String<"method">, Int<"amount">, String<"status">, String<"bankName">, String<"voucherTypeOverride">>>, String<"achChecksum">>;
	using EmployeeEmployeeEmployeeEventsArraySchema = Object<"employeeEvents", Array<"employeeEvent", Object_<String<"eventCode">, String<"effectiveDate">, String<"description">, String<"actionDate">, String<"comment">>>>;
	using EmployeeEmpolyeeHealthSchema = Object<"health", Array<"allergy", String_>, Array<"condition", String_>, Array<"program", String_>, String<"height">, String<"weight">, String<"bloodType">, String<"bloodRh">, Bool<"bloodDonor">, String<"donateDateLast">, String<"donateDateNext">, String<"drugTestResult">, String<"drugTestDateLast">, String<"drugTestDateNext">, String<"audioTestResult">, String<"audioTestDateLast">, String<"audioTestDateNext">, String<"physExamResult">, String<"physExamDateLast">, String<"physExamDateNext">>;
	using EmployeeEmployeeSchoolSchema = Object_<String<"schoolName">, String<"schoolYear">, Bool<"graduated">, String<"gradYear">, String<"degreeEarned">, String<"schoolComment">>;
	using EmployeeClientSchema = Object<"client", String<"employeeStatus">, String<"statusClass">, String<"statusDate">, String<"employeeType">, String<"typeClass">, String<"typeDate">, Array<"transferInfo", Object_<String<"transferStatus">, String<"transferEmployeeId">, String<"transferClientId">, String<"transferDate">>>, Array<"property", Object_<String<"propertyCode">, String<"propertyIssueDate">, String<"propertySerialNumber">, String<"propertyValue">, String<"propertyReturnDate">, String<"propertyComment">>>, Array<"hawaiiMedWaivers", Object_<String<"medWaiverYear">, String<"medWaiverReason">>>, String<"jobCode">, String<"jobStartDate">, String<"firstHireDate">, String<"lastHireDate">, Bool<"flsaExempt">, Array<"cafeteriaPlan", Object_<String<"planAmt">, String<"planEffectiveDate">>>, String<"seniorityDate">, Bool<"officer">, Bool<"workCompOfficer">, Bool<"scorpOwner">, Bool<"businessOwner">, String<"termReasonCode">, String<"termExplanation">, String<"lastStatusChangeDate">, String<"lastJobChangeDate">, String<"newHireReportDate">, Bool<"employee1099">, String<"clientEmployeeId">, String<"clockNum">, String<"primaryEmailSource">, String<"benefitEmail">, String<"rehireOk">, String<"returnDate">, String<"workPhone">, String<"workPhoneExt">, String<"probationDate">, Bool<"courtOrderMedical">, Bool<"childSpouseSupport">, Bool<"childSpouseArrears">, String<"addressLine1">, String<"addressLine2">, String<"city">, String<"state">, String<"zip">, String<"zipSuffix">, String<"handbookMailDate">, String<"handbookRecvDate">, String<"unionCode">, String<"unionDate">, String<"subCostCenter">, Bool<"electronicOnboard">, String<"electronicOnboardDate">, String<"reportsTo">, Bool<"onboardInProgress">, Bool<"backgroundTest">, String<"backgroundTestDate">, Bool<"workersCompExempt">, String<"genderDesignation">, String<"preferredPronoun">, String<"clientChecksum">, String<"homeLocation">, String<"homeDivision">, String<"homeDepartment">, String<"workShift">, String<"projCostCenter">, String<"workGroupCode">, String<"benefitGroup">, String<"retirementBenefitGroup">, String<"workEmail">, String<"electronic1095C">, String<"manager">, String<"selfIdDisabilityForm">, String<"applicantId">>;
	using EmployeeContactInformationSchema = Object<"contactInformation", String<"addressLine1">, String<"addressLine2">, String<"city">, String<"state">, String<"zipcode">, String<"zipSuffix">, String<"county">, String<"schoolDistrict">, String<"homePhone">, String<"mobilePhone">, String<"emailAddress">>;
	using EmployeeEmployeeSchema = Object_<String<"id">, String<"lastName">, String<"firstName">, String<"middleInitial">, String<"preferredName">, EmployeePersonSchema, EmployeeContactInformationSchema, EmployeeClientSchema, EmployeeEmployeeCompensationSchema, EmployeeEmployeeDirectDepositSchema, EmployeeEmployeeHsaSchema, EmployeeEmployeeEmployeeEventsArraySchema, EmployeeEmpolyeeHealthSchema, Object<"skillsAndEducation", String<"skillChecksum">, Array<"skill", Object_<String<"skillCode">, String<"competencyLevel">, String<"certifyDate">, String<"skillComment">, String<"renewDate">>, Array<"school", EmployeeEmployeeSchoolSchema>>, Bool<"osha10Certified">>, Array<"absenceJournalId", String_>, Bool<"benefitEnrollmentCompleted">, Array<"scheduledDeduction", Object_<String<"deductionCode">, String<"checkStubDescription">, String<"status">, Int<"amount">, String<"startDate">, String<"stopDate">, String<"cutbackPayeeCode">>, Array<"newHireQuestions", Object_<String<"code">, String<"description">, String<"answer">>>, Object<"enrollmentInProgress", Bool<"enrollmentInProgress">, String<"enrollmentType">>>>;
	using EmployeeArraySchema = Array<"employee", EmployeeEmployeeSchema>;
	using EmployeeSchema = Object_<String<"errorCode">, String<"errorMessage">, EmployeeArraySchema, Array<"infoMessage", String_>, String<"updateMessage">>;

	std::string bigassJson = R"---({"errorCode":"string","errorMessage":"string","extension":{"any":{}},"employee":[{"id":"string","lastName":"string","firstName":"string","middleInitial":"string","preferredName":"string","person":{"ethnicCode":"string","birthDate":"string","driverLicenseId":"string","cobraSSN":"string","employerIds":["string"],"cobraOnly":true,"geoCode":"string","maritalStatus":"string","marriedDate":"string","smoker":"string","peoStartDate":"string","handicapped":"string","blind":"string","veteran":"string","vietnamVet":"string","disabledVet":"string","hrpCitizenshipCountry":"string","deceased":true,"gender":"string","taxZipCode":"string","highlyCompensated":true,"keyEmployee":true,"overrideGeoCode":"string","overrideGeoEndDate":"string","nickname":"string","deceasedDate":"string","driverState":"string","driverLicenseExpireDate":"string","driverLicenseClass":"string","ohioFormC112":true,"unincorporatedResidence":true,"w2AddressLine1":"string","w2AddressLine2":"string","w2City":"string","w2State":"string","w2PostalCode":"string","w2ElecForm":true,"w2ElecFormDate":"string","legacyEmployeeId":"string","hispanic":true,"agricultureWorker":true,"otherProtectedVet":"string","activeDutyBadgeVet":"string","recentlySeparatedVet":"string","serviceMedalVet":"string","preferredLanguage":"string","emergencyContact":[{"contactName":"string","contactInfo":"string","contactRelationship":"string","contactType":"string"}],"userId":"string","autoPolicies":[{"autoInsurancePolicyId":"string","autoInsuranceCarrierName":"string","autoInsuranceHolderName":"string","autoInsurancePolicyStartDate":"YYYY-MM-DD","autoInsurancePolicyExpDate":"YYYY-MM-DD"}],"vehicleDetails":[{"vehicleMake":"string","vehicleModel":"string","vehicleYear":"string","vehicleAddDate":"YYYY-MM-DD","vehicleRemoveDate":"YYYY-MM-DD","vehicleRegNo":"string"}],"personChecksum":"string"},"contactInformation":{"addressLine1":"string","addressLine2":"string","city":"string","state":"string","zipcode":"string","zipSuffix":"string","county":"string","schoolDistrict":"string","homePhone":"string","mobilePhone":"string","emailAddress":"string"},"client":{"employeeStatus":"string","statusClass":"string","statusDate":"string","employeeType":"string","typeClass":"string","typeDate":"string","transferInfo":[{"transferStatus":"string","transferEmployeeId":"string","transferClientId":"string","transferDate":"string"}],"property":[{"propertyCode":"string","propertyIssueDate":"string","propertySerialNumber":"string","propertyValue":"string","propertyReturnDate":"string","propertyComment":"string"}],"hawaiiMedWaivers":[{"medWaiverYear":"string","medWaiverReason":"string"}],"jobCode":"string","jobStartDate":"string","firstHireDate":"string","lastHireDate":"string","flsaExempt":true,"cafeteriaPlan":[{"planAmt":"string","planEffectiveDate":"string"}],"seniorityDate":"string","officer":true,"workCompOfficer":true,"scorpOwner":true,"businessOwner":true,"termReasonCode":"string","termExplanation":"string","lastStatusChangeDate":"string","lastJobChangeDate":"string","newHireReportDate":"string","employee1099":true,"clientEmployeeId":"string","clockNum":"string","primaryEmailSource":"string","benefitEmail":"string","rehireOk":"string","returnDate":"string","workPhone":"string","workPhoneExt":"string","probationDate":"string","courtOrderMedical":true,"childSpouseSupport":true,"childSpouseArrears":true,"addressLine1":"string","addressLine2":"string","city":"string","state":"string","zip":"string","zipSuffix":"string","handbookMailDate":"string","handbookRecvDate":"string","unionCode":"string","unionDate":"string","subCostCenter":"string","electronicOnboard":true,"electronicOnboardDate":"string","reportsTo":"string","onboardInProgress":true,"backgroundTest":true,"backgroundTestDate":"string","workersCompExempt":true,"genderDesignation":"string","preferredPronoun":"string","clientChecksum":"string","homeLocation":"string","homeDivision":"string","homeDepartment":"string","workShift":"string","projCostCenter":"string","workGroupCode":"string","benefitGroup":"string","retirementBenefitGroup":"string","workEmail":"string","electronic1095C":"string","manager":"string","selfIdDisabilityForm":"string","applicantId":"string"},"compensation":{"ssn":"string","payGroup":"string","payPeriod":"string","payPeriodInfo":[{"periodCode":"string","periodBase":"string"}],"defaultHours":"string","payMethod":"string","allocationTemplateId":"string","payAllocation":[{"locationCode":"string","percent":"string","job":"string","deptCode":"string","project":"string","division":"string"}],"standardHours":"string","stateTax":[{"stateCode":"string","nonResCertFiled":"string","filingStatus":"string","primaryAllowance":"string","secondaryAllowance":"string","exemptAmount":"string","supplExemptAmount":"string","overrideType":"string","overrideAmount":"string","reduceAddlWithholding":"string","fixedWithholding":"string","alternateCalc":"string","stateW4Filed":"string","stateW4Year":"string","multipleJobs":false,"dependents":0,"otherIncome":0,"deductions":0,"nonResCertYear":"string"}],"hourlyPay":"string","salary":"string","workShift":"string","paidThruDate":"string","eicFileStatus":"string","fedFilingStatus":"string","fedWithholdAllowance":"string","fedOverrideType":"string","fedOverrideAmt":"string","fedTaxCalc":"string","fedMultipleJobs":true,"fedDependents":0,"fedOtherIncome":0,"fedDeductions":0,"hourlyPayRate":"string","hourlyPayPeriod":"string","deliverCheckHome":true,"autoAcceptTimeSheet":true,"longPayTable":"string","longBasisDate":"string","annualPay":"string","effectiveDate":"string","localTax":[{"authId":"string","filingStatus":"string","primaryAllowance":"string","nonResCert":"string","addlWithheld":"string"}],"lastChangePct":"string","lastChangeAmt":"string","lastPayDate":"string","quartile":"string","compaRatio":"string","benefitsBasePay":"string","perDiemPay":"string","taxCreditEmp":true,"autoPayCode":"string","altPayRate":["string"],"pensionStatus":"string","profitSharing":"string","eeImage":[{"masterId":"string","verifyDateTime":"2019-08-24T14:15:22Z","verifyUserId":"string"}],"wagePlanCA":true,"perfReviewLastDate":"string","perfReviewNextDate":"string","i9imageAudit":[{"auditDate":"string","auditUser":"string","auditId":"string","auditStep":"string"}],"perfReviewLastRating":"string","compReviewNextDate":"string","perfReviewLastTitle":"string","fedW5Year":"string","fedW5Filed":true,"fedW4Filed":true,"fedW4Year":"string","lockIns":[{"state":"string","stateDate":"string","authUser":"string"}],"formI9Filed":true,"formI9RenewDate":"string","notes":"string","ficaExempt":true,"benefitsPerHour":"string","perfAgreement":true,"payRate2":"string","payRate3":"string","payRate4":"string","payChangeLastDate":"string","overrideWorkGeocode":"string","familyMemberMI":"string","probationCodeMO":"string","electronicPayStub":true,"healthInsVT":true,"lastWorkedDate":"string","irsLockInDate":"string","nonResAlien":true,"alienRegExpDate":"string","benefitSalary":"string","citizenshipStatus":"string","firstPayPeriodHours":"string","compensationChecksum":"string","providerNotifiedOn":"string","alternateId":"string","w8Filed":false},"directDeposit":{"achStatus":"string","voucherType":"string","suppressAccountPrint":true,"achVoucher":[{"transitNum":"string","accountNum":"string","accountType":"string","method":"string","amount":0,"limit":0,"status":"string","bankName":"string","voucherTypeOverride":"string"}],"directDepositChecksum":"string"},"hsa":{"hsaDepositSettings":[{"transitNum":"string","accountNum":"string","accountType":"string","method":"string","amount":0,"status":"string","bankName":"string","voucherTypeOverride":"string"}],"achChecksum":"string"},"employeeEvents":{"employeeEvent":[{"eventCode":"string","effectiveDate":"string","description":"string","actionDate":"string","comment":"string"}]},"health":{"allergy":["string"],"condition":["string"],"program":["string"],"height":"string","weight":"string","bloodType":"string","bloodRh":"string","bloodDonor":true,"donateDateLast":"string","donateDateNext":"string","drugTestResult":"string","drugTestDateLast":"string","drugTestDateNext":"string","audioTestResult":"string","audioTestDateLast":"string","audioTestDateNext":"string","physExamResult":"string","physExamDateLast":"string","physExamDateNext":"string"},"skillsAndEducation":{"skillChecksum":"string","skill":[{"skillCode":"string","competencyLevel":"string","certifyDate":"2019-08-24","skillComment":"string","renewDate":"2019-08-24"}],"school":[{"schoolName":"string","schoolYear":"string","graduated":false,"gradYear":"string","degreeEarned":"string","schoolComment":"string"}],"osha10Certified":true},"absenceJournalId":["string"],"benefitEnrollmentCompleted":true,"scheduledDeduction":[{"deductionCode":"string","checkStubDescription":"string","status":"string","amount":0,"startDate":"string","stopDate":"string","cutbackPayeeCode":"string"}],"newHireQuestions":[{"code":"string","description":"string","answer":"string"}],"enrollmentInProgress":{"enrollmentInProgress":true,"enrollmentType":"string"}}],"infoMessage":["string"],"updateMessage":"string"})---";

	iss.str(bigassJson);
	typename EmployeeSchema::rettype prism;
	try {
		prism = const_json::parse<EmployeeSchema>(iss);
	}
	catch (const_json::err::NotAllMembersPresentError& e) {
		std::cout << e.path << std::endl;
		for (auto it = e.membersAbsent.begin(); it != e.membersAbsent.end(); ++it) {
			std::cout << '\t' << *it << std::endl;
		}
		return 1;
	}
	catch (const_json::err::BadTypeError& e) {
		std::cout << "Bad Type Error at: " << e.path << std::endl;
		return 1;
	}
	catch (const_json::err::MalformedInputError) {
		std::cout << "Malformed Input Error at char: " << static_cast<size_t>(iss.tellg()) << std::endl;
		return 1;
	}

	std::cout << "\n\n" << std::endl;

	auto& employeeArray = const_json::getMember<EmployeeSchema, "employee">(prism);
	for (auto it = std::begin(employeeArray); it != std::end(employeeArray); ++it) {
		using Schema = typename const_json::get_member_schema<EmployeeSchema, "employee">::value::element_schema;
		auto& id2 = const_json::getMember<Schema, "id">(*it);
		id2 = "And now for something completely different";
	}

	const_json::dump<EmployeeSchema, fmt>(prism, std::cout);

	std::cout << std::endl;

	return 0;
}