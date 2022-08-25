#ifndef SENDGRID_H
#define SENDGRID_H

#include <string>
#include <vector>
#include <map>
#include "CJsonObject.hpp"

using namespace std;
using namespace neb;

namespace SendGrid
{
    template <typename T>
    CJsonObject hashToJson(map<string, T> &m)
    {
        CJsonObject obj;
        for (auto it = m.begin(); it != m.end(); it++)
        {
            obj.Add(it->first, it->second);
        }
        return obj;
    }

    template <typename T>
    vector<CJsonObject> listToJson(vector<T> &list)
    {
        vector<CJsonObject> vec;
        for (T value : list)
        {
            CJsonObject obj;
            obj.Add(value);
            vec.push_back(obj);
        }

        return vec;
    }

    template <typename T>
    vector<CJsonObject> listToJson2(vector<T> &list)
    {
        vector<CJsonObject> vec;
        for (T value : list)
        {
            CJsonObject obj;
            obj.Add(value.toJson());
            vec.push_back(obj);
        }

        return vec;
    }

    static const string SendGridMimeTypeHTML = "text/html";
    static const string SendGridMimeTypeText = "text/plain";

    // An object allowing you to specify how to handle unsubscribes.
    struct ASM
    {
        // Gets or sets the unsubscribe group to associate with this email.
        int groupId;

        /// Gets or sets an array containing the unsubscribe groups that you would like to be displayed on the unsubscribe preferences page.
        /// https://sendgrid.com/docs/User_Guide/Suppressions/recipient_subscription_preferences.html
        vector<int> groupsToDisplay;

        neb::CJsonObject toJson()
        {
            neb::CJsonObject oJson;
            oJson.Add("groupId", std::to_string(groupId));
            oJson.AddEmptySubArray("groupsToDisplay");
            for (size_t i = 0; i < groupsToDisplay.size() - 1; i++)
            {
                oJson["groupsToDisplay"].Add(std::to_string(groupsToDisplay[i]));
            }
            return oJson;
        }
    };

    // Gets or sets an array of objects in which you can specify any attachments you want to include.
    struct Attachment
    {
        /// Gets or sets the Base64 encoded content of the attachment.
        string content;
        // Gets or sets the mime type of the content you are attaching. For example, application/pdf or image/jpeg.
        string type;
        // Gets or sets the filename of the attachment.
        string filename;
        // Gets or sets the content-disposition of the attachment specifying how you would like the attachment to be displayed. For example, "inline" results in the attached file being displayed automatically within the message while "attachment" results in the attached file requiring some action to be taken before it is displayed (e.g. opening or downloading the file). Defaults to "attachment". Can be either "attachment" or "inline".
        string disposition;
        // Gets or sets a unique id that you specify for the attachment. This is used when the disposition is set to "inline" and the attachment is an image, allowing the file to be displayed within the body of your email. Ex: <img src="cid:ii_139db99fdb5c3704"></img>
        string contentId;

        Attachment() {}

        Attachment(string content, string type, string filename, string disposition, string contentId)
        {
            this->content = content;
            this->type = type;
            this->filename = filename;
            this->disposition = disposition;
            this->contentId = contentId;
        }

        neb::CJsonObject toJson()
        {
            neb::CJsonObject oJson;
            oJson.Add("content", content);
            oJson.Add("type", type);
            oJson.Add("filename", filename);
            oJson.Add("disposition", disposition);
            oJson.Add("contentId", contentId);
            return oJson;
        }
    };

    // Gets or sets the address specified in the mail_settings.bcc object will receive a blind carbon copy (BCC) of the very first personalization defined in the personalizations array.
    struct BCCSettings
    {
        /// Gets or sets a value indicating whether this setting is enabled.
        bool enable;

        /// Gets or sets the email address that you would like to receive the BCC.
        string email;

        neb::CJsonObject toJson()
        {
            neb::CJsonObject oJson;
            oJson.Add("enable", std::to_string(enable));
            oJson.Add("email", email);
            return oJson;
        }
    };

    /// Specifies the content of your email. You can include multiple mime types of content, but you must specify at least one. To include more than one mime type, simply add another object to the array containing the type and value parameters. If included, text/plain and text/html must be the first indices of the array in this order. If you choose to include the text/plain or text/html mime types, they must be the first indices of the content array in the order text/plain, text/html.*Content is NOT mandatory if you using a transactional template and have defined the template_id in the Request
    struct Content
    {
        Content() {}

        Content(string type, string value)
        {
            this->type = type;
            this->value = value;
        }
        /// Gets or sets the mime type of the content you are including in your email. For example, text/plain or text/html.
        string type;
        /// Gets or sets the actual content of the specified mime type that you are including in your email.
        string value;
        neb::CJsonObject toJson()
        {
            neb::CJsonObject oJson;
            oJson.Add("type", type);
            oJson.Add("value", value);
            return oJson;
        }
    };

    struct EmailAddress
    {

        /// Initializes a new instance of the <see cref="EmailAddress"/> class.
        EmailAddress() {}

        EmailAddress(string email, string name = "")
        {
            this->email = email;
            this->name = name;
        }

        /// Gets or sets the name of the sender or recipient.
        string name;
        /// Gets or sets the email address of the sender or recipient.
        string email;
        neb::CJsonObject toJson()
        {
            neb::CJsonObject oJson;
            oJson.Add("name", name);
            oJson.Add("email", email);
            return oJson;
        }
    };

    /// The default footer that you would like appended to the bottom of every email.
    struct FooterSettings
    {
        /// Gets or sets a value indicating whether this setting is enabled.
        bool enable;
        /// Gets or sets the plain text content of your footer.
        string text;
        /// Gets or sets the HTML content of your footer.
        string html;
        neb::CJsonObject toJson()
        {
            neb::CJsonObject oJson;
            oJson.Add("enable", std::to_string(enable));
            oJson.Add("text", text);
            oJson.Add("html", html);
            return oJson;
        }
    };

    /// Helper class for plain html mime types
    struct HtmlContent : public Content
    {
        HtmlContent(string value)
        {
            this->type = SendGridMimeTypeHTML;
            this->value = value;
        }
    };

    /// An array of messages and their metadata. Each object within personalizations can be thought of as an envelope - it defines who should receive an individual message and how that message should be handled. For more information, please see our documentation on Personalizations. Parameters in personalizations will override the parameters of the same name from the message level.
    /// https://sendgrid.com/docs/Classroom/Send/v3_Mail_Send/personalizations.html
    struct Personalization
    {
        /// Gets or sets an array of recipients. Each email object within this array may contain the recipient’s name, but must always contain the recipient’s email.
        vector<EmailAddress> to;

        /// Gets or sets an array of recipients who will receive a copy of your email. Each email object within this array may contain the recipient’s name, but must always contain the recipient’s email.
        vector<EmailAddress> cc;

        /// Gets or sets an array of recipients who will receive a blind carbon copy of your email. Each email object within this array may contain the recipient’s name, but must always contain the recipient’s email.
        vector<EmailAddress> bcc;

        /// Gets or sets the subject line of your email.
        string subject;

        /// Gets or sets the object allowing you to specify specific handling instructions for your email.
        map<string, string> headers;

        /// Gets or sets an object following the pattern "substitution_tag":"value to substitute". All are assumed to be strings. These substitutions will apply to the content of your email, in addition to the subject and reply-to parameters.
        /// You may not include more than 100 substitutions per personalization object, and the total collective size of your substitutions may not exceed 10,000 bytes per personalization object.
        map<string, string> substitutions;

        /// Gets or sets the values that are specific to this personalization that will be carried along with the email, activity data, and links. Substitutions will not be made on custom arguments. personalizations[x].custom_args will be merged with message level custom_args, overriding any conflicting keys. The combined total size of the resulting custom arguments, after merging, for each personalization may not exceed 10,000 bytes.
        map<string, string> customArgs;

        /// Gets or sets a unix timestamp allowing you to specify when you want your email to be sent from SendGrid. This is not necessary if you want the email to be sent at the time of your API request.
        uint64 sendAt = 0;

        neb::CJsonObject dynamicTemplateData;

        void SetDynamicTemplateData(neb::CJsonObject obj)
        {
            dynamicTemplateData = obj;
        }

        neb::CJsonObject toJson()
        {
            neb::CJsonObject obj;
            if (!to.empty())
            {
                obj.AddEmptySubArray("to");
                for (int i = 0; i < to.size(); i++)
                {
                    obj["to"].Add(to[i].toJson());
                }
            }

            if (!cc.empty())
            {
                obj.AddEmptySubArray("cc");
                for (int i = 0; i < cc.size(); i++)
                {
                    obj["cc"].Add(cc[i].toJson());
                }
            }

            if (!bcc.empty())
            {
                obj.AddEmptySubArray("bcc");
                for (int i = 0; i < bcc.size(); i++)
                {
                    obj["bcc"].Add(bcc[i].toJson());
                }
            }

            if (!subject.empty())
            {
                obj.Add("subject", subject);
            }
            if (!substitutions.empty())
            {
                obj.Add("substitutions", hashToJson(substitutions));
            }
            if (!customArgs.empty())
            {
                obj.Add("custom_args", hashToJson(customArgs));
            }
            if (sendAt > 0)
            {
                obj.Add("send_at", sendAt);
            }
            obj.Add("dynamic_template_data", dynamicTemplateData);
            return obj;
        }
    };

    /// This allows you to send a test email to ensure that your request body is valid and formatted correctly. For more information, please see our Classroom.
    /// https://sendgrid.com/docs/Classroom/Send/v3_Mail_Send/sandbox_mode.html
    struct SandboxMode
    {
        /// Gets or sets a value indicating whether this setting is enabled.
        bool enable;

        neb::CJsonObject toJson()
        {
            neb::CJsonObject oJson;
            oJson.Add("enable", std::to_string(enable));
            return oJson;
        }
    };

    /// A collection of different mail settings that you can use to specify how you would like this email to be handled.
    struct MailSettings
    {
        /// Gets or sets the address specified in the mail_settings.bcc object will receive a blind carbon copy (BCC) of the very first personalization defined in the personalizations array.
        BCCSettings *bccSettings = NULL;
        /// Gets or sets the default footer that you would like appended to the bottom of every email.
        FooterSettings *footerSettings = NULL;
        /// Gets or sets the ability to send a test email to ensure that your request body is valid and formatted correctly. For more information, please see our Classroom.
        /// https://sendgrid.com/docs/Classroom/Send/v3_Mail_Send/sandbox_mode.html
        SandboxMode *sandboxMode = NULL;

        neb::CJsonObject toJson()
        {
            neb::CJsonObject oJson;
            if (bccSettings)
            {
                oJson.Add("bccSettings", bccSettings->toJson());
            }
            if (footerSettings)
            {
                oJson.Add("footerSettings", footerSettings->toJson());
            }
            if (sandboxMode)
            {
                oJson.Add("sandboxMode", sandboxMode->toJson());
            }
            return oJson;
        }
    };

    struct PlainTextContent : public Content
    {
        PlainTextContent(string value)
        {
            this->type = SendGridMimeTypeText;
            this->value = value;
        }
    };
}

#endif