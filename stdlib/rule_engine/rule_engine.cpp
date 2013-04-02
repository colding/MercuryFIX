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

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <string.h>
#include <errno.h>
#include "rule_engine.h"

static bool
eval_string_case(const void * const lval,
		 const enum Evaluation cond,
		 const void * const rval)
{
	switch (cond)
	{
	case eEqual:
		return (0 == strcmp((const char*)lval, (const char*)rval));
	case eGreaterThan:
		return (0 < strcmp((const char*)lval, (const char*)rval));
	case eLessThan:
		return (0 > strcmp((const char*)lval, (const char*)rval));
	case eNotEqual:
		return (0 != strcmp((const char*)lval, (const char*)rval));
	case eTrue:
		return true;
	case eFalse:
		return false;
	default:
		assert(false);
	}
	return false;
}

static bool
eval_string_no_case(const void * const lval,
		    const enum Evaluation cond,
		    const void * const rval)
{
	switch (cond)
	{
	case eEqual:
		return (0 == strcasecmp((const char*)lval, (const char*)rval));
	case eGreaterThan:
		return (0 < strcasecmp((const char*)lval, (const char*)rval));
	case eLessThan:
		return (0 > strcasecmp((const char*)lval, (const char*)rval));
	case eNotEqual:
		return (0 != strcasecmp((const char*)lval, (const char*)rval));
	case eTrue:
		return true;
	case eFalse:
		return false;
	default:
		assert(false);
	}
	return false;
}

// Not nearly good enough. Use IEEE754-2008 standard spec
// static bool
// eval_ieee754(const void * const lval,
// 	     const enum Evaluation cond,
// 	     const void * const rval)
// {
// 	switch (cond)
// 	{
// 	case eEqual:
// 		return (*(double*)lval == *(double*)rval);
// 	case eGreaterThan:
// 		return (*(double*)lval == *(double*)rval);
// 	case eLessThan:
// 		return (*(double*)lval == *(double*)rval);
// 	case eNotEqual:
// 		return (*(double*)lval == *(double*)rval);
// 	case eTrue:
// 		return true;
// 	case eFalse:
// 		return false;
// 	default:
// 		assert(false);
// 	}
//	return false;
//}

template<class T> bool
eval_generic(const void * const lval,
	     const enum Evaluation cond,
	     const void * const rval)
{
	switch (cond)
	{
	case eEqual:
		return (*(T*)lval == *(T*)rval);
	case eGreaterThan:
		return (*(T*)lval > *(T*)rval);
	case eLessThan:
		return (*(T*)lval < *(T*)rval);
	case eNotEqual:
		return (*(T*)lval != *(T*)rval);
	case eTrue:
		return true;
	case eFalse:
		return false;
	default:
		assert(false);
	}
	return false;
}

static bool
evaluate_criteria(char **expression,
		  const Event * const incident,
		  const CriteriaCache * const criteria_cache)
{
//	const CriteriaCache & cc = *criteria_cache;

	// extract criteria ID
	const unsigned int criteria_id = strtoul(*expression, expression, 16);
	if (!criteria_id && (EINVAL == errno))
	{
		d("BUUUUG! Criteria ID = %u", criteria_id);
		return false; // BUG - log and scream or else?
	}

	// find criteria
	CriteriaCache::const_iterator cit = criteria_cache->find(criteria_id);
	if (criteria_cache->end() == cit)
	{
		d("Error! Criteria ID = %u", criteria_id);
		return false; // configuration error - log
	}

	// find TagInstance in Event
	Event::const_iterator eit = incident->find(cit->second.tag);
	if (incident->end() == eit)
	{
		d("Tag not found in event: %#.8x", cit->second.tag);
		return false;
	}
	switch (cit->second.tag >> 24)
	{
	case vt_boolean:
		return eval_generic<bool>(&eit->second.int_value,
					  cit->second.cond,
					  &cit->second.int_value);
	case vt_uint8:
		return eval_generic<uint8_t>(&eit->second.int_value,
					     cit->second.cond,
					     &cit->second.int_value);
		break;
	case vt_int8:

		return eval_generic<int8_t>(&eit->second.int_value,
					    cit->second.cond,
					    &cit->second.int_value);
	case vt_uint16:
		return eval_generic<uint16_t>(&eit->second.int_value,
					      cit->second.cond,
					      &cit->second.int_value);
	case vt_int16:
		return eval_generic<int16_t>(&eit->second.int_value,
					     cit->second.cond,
					     &cit->second.int_value);
	case vt_uint32:
		return eval_generic<uint32_t>(&eit->second.int_value,
					      cit->second.cond,
					      &cit->second.int_value);
	case vt_int32:
		return eval_generic<int32_t>(&eit->second.int_value,
					     cit->second.cond,
					     &cit->second.int_value);
	case vt_ASCIIString:
		return eval_string_case(eit->second.ptr_value,
					cit->second.cond,
					cit->second.ptr_value);
	case vt_ASCIIStringNoCase:
		return eval_string_no_case(eit->second.ptr_value,
					   cit->second.cond,
					   cit->second.ptr_value);
	// case vt_ieee754:
	// 	break;
	// case vt_UTF8String:
	// 	return eval_string_utf8((*incident)[tag].value,
	// 				(*criteria_cache)[criteria_id].cond,
	// 				(*criteria_cache)[criteria_id].fixed_value);
	// 	break;
	// case vt_UTF8StringNoCase:
	// 	return eval_string_utf8_no_case((*incident)[tag].value,
	// 					(*criteria_cache)[criteria_id].cond,
	// 					(*criteria_cache)[criteria_id].fixed_value);
	// 	break;
	default:
		d("BUG!\n");
		break;
	}

	return false; // BUG - log, scream and crash?
}

static inline void
skip_forward_over_block(char **expression)
{
	unsigned int par_cnt = 1;

	do {
		++(*expression);
		switch (**expression) {
		case PAR_END:
			--par_cnt;
			if (!par_cnt)
				return;
		case PAR_START:
			++par_cnt;
			break;
		default:
			break;
		}
	} while (**expression);
}


static inline void
skip_forward_to_OR(char **expression)
{
	do {
		switch (**expression) {
		case '\0':
		case OR:
			return;
		case PAR_START:
			skip_forward_over_block(expression);
		default:
			break;
		}
		++(*expression);
	} while (**expression);
}

static inline void
skip_forward_to_AND(char **expression)
{
	do {
		switch (**expression) {
		case '\0':
		case AND:
			return;
		case PAR_START:
			skip_forward_over_block(expression);
		default:
			break;
		}
		++(*expression);
	} while (**expression);
}

static bool
evaluate_expression(char **expression,
		    const Event * const incident,
		    const CriteriaCache * const criteria_cache)
{
	bool retv = false;

	while (**expression) {
		if (PAR_START == **expression)
			++(*expression);

		switch (**expression) {
		case '\0':
			return retv;
		case PAR_END:
			++(*expression);
			continue;
		case AND:
			++(*expression);
			if (!retv)
			{
				skip_forward_to_OR(expression);
			}
			if (PAR_START == **expression)
			{
				retv = retv && evaluate_expression(expression, incident, criteria_cache);
			}
			else
			{
				retv = retv && evaluate_criteria(expression, incident, criteria_cache);
			}
			break;
		case OR:
			++(*expression);
			if (retv) {
				skip_forward_to_AND(expression);
				continue;
			}
			if (PAR_START == **expression)
			{
				retv = retv || evaluate_expression(expression, incident, criteria_cache);
			}
			else
			{
				retv = retv || evaluate_criteria(expression, incident, criteria_cache);
			}
			break;
		case PAR_START:
			retv = evaluate_expression(expression, incident, criteria_cache);
			break;
		default:
			retv = evaluate_criteria(expression, incident, criteria_cache);
			break;
		}
	}

	return retv;
}

const char*
evaluate_incident_with_rules(const Event * const incident,
			     const RuleSet * const rules,
			     const CriteriaCache * const criteria_cache)
{
	RuleSet::size_type len = rules->size();
	if (!len)
		return NULL;

	bool retv;
	char *expr;
	for (RuleSet::size_type n = 0; n < len; ++n) {
		expr = (char*)(*rules)[n].expression;
		d("Initial expression: %s\n", expr);
		retv = evaluate_expression(&expr, incident, criteria_cache);
		retv = (*rules)[n].negate ? !retv : retv;
		if (retv)
			return (*rules)[n].id;
		else
			d("Rule %s did not match", (*rules)[n].id);
	}

	return NULL;
}
