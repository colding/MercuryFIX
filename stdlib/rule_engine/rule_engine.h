/*
 *    Copyright (C) 2013, Jules Colding <jcolding@gmail.com>.
 *
 *    All Rights Reserved.
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *     (1) Redistributions of source code must retain the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer.
 * 
 *     (2) Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *     
 *     (3) Neither the name of the copyright holder nor the names of
 *     its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <algorithm>
#include <vector>
#include <map>

#define d(msg__, ...) do { printf("%s@%d: " msg__"\n", __FILE__, __LINE__, ## __VA_ARGS__); } while (0)

// logical operators in expression
const char AND = '+';
const char OR = '|';
const char PAR_START = '(';
const char PAR_END = ')';


enum ValueType
{
	vt_boolean           = 0x00,
	vt_uint8             = 0x01,
	vt_int8              = 0x02,
	vt_uint16            = 0x03,
	vt_int16             = 0x04,
	vt_uint32            = 0x05,
	vt_int32             = 0x06,
	vt_uint64            = 0x07,
	vt_int64             = 0x08,
	vt_ieee754           = 0x09,
	vt_ASCIIString       = 0x0A,
	vt_ASCIIStringNoCase = 0x0B,
	vt_UTF8String        = 0x0C,
	vt_UTF8StringNoCase  = 0x0D,
};

enum Evaluation
{
	eEqual,
	eGreaterThan,
	eGreaterThanOrEqual,
	eLessThan,
	eLessThanOrEqual,
	eNotEqual,
	eTrue,
	eFalse
};

/*
 * This holds a real world situation, a.k.a an event, to be evaluated
 * by a RuleSet.
 */
struct TagInstance
{
	uint64_t int_value; // internal fixed integer lvalue
	void *ptr_value;    // internal fixed pointer lvalue
};
typedef std::map< /*tag*/ uint32_t, /*value*/ struct TagInstance> Event;

/*
 * A single criteria. It has a pointer to an evaluator function, a
 * logical condition, a tag identifying the entity being evaluated, an
 * ID and an intertal fixed rvalue which will be compared to some
 * lvalue.
 *
 * A tag is a 32 bits unsigned integer. The first 8 bits denotes the
 * type and the next 24 the tag ID. So there is a possible 256 types
 * and 16777216 possible tag IDs. No one will ever need more (famous
 * last words...).
 *
 * The CriteriaCache holds all criteria for a given RuleDomain.
 */
struct Criteria
{
	enum Evaluation cond; // what comparisson operation to do
	uint32_t tag;         // tag of fixed value

	uint64_t int_value; // internal fixed integer rvalue
	void *ptr_value;    // internal fixed pointer rvalue
};
typedef std::map< /*CriteriaID*/ unsigned int, struct Criteria> CriteriaCache;

/*
 * A Rule is an expression which is a series of Criteria connected by
 * logical operators and grouped by paranthesises. A Rule has an
 * action, a priority and may be negated.
 */
struct Rule
{
	int negate;            // negate this rule?
	unsigned int priority; // from 0 to 0xFFFFFFFF, the higher the more important
	unsigned int action;   // action ID, i.e. what to do when the rule is matched
	char *expression;      // null terminated UTF8 string - logical operators and Criteria IDs in hex
	char *id;              // Some Id, GUID?
};

/*
 * A RuleSet is a set of Rules. A RuleSet has an ID. An example of a
 * RuleSet could be a commission set, or commission group as it is
 * called elsewhere...
 */
typedef std::vector<struct Rule> RuleSet;

static inline bool
lrule_is_less_important_than_rrule(const Rule & l_rule, const Rule & r_rule)
{
	return (l_rule.priority < r_rule.priority);
}

static inline void
sort_rule_set(RuleSet & rule_set)
{
	std::sort(rule_set.begin(), rule_set.end(), lrule_is_less_important_than_rrule);
}


/*
 * A RuleDomain are all appropiate set of rules for a given domain. It
 * could e.g. be all commission rules, all routing rules, all FIX
 * rules or all FIX server configuration rules.
 */
typedef std::map</*RuleSetID*/unsigned int, RuleSet> RuleDomain;

/*
 * Evaluates an Event, or instantiation of tags, using a specific
 * RuleSet, The CriteriaCache must contain all criteria used in the
 * RuleSet. The function returns the ID of the first matching rule.
 */
extern const char*
evaluate_incident_with_rules(const Event * const incident,
			     const RuleSet * const rules,
			     const CriteriaCache * const criteria_cache);
