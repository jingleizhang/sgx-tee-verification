#ifndef SENDGRIDMSG_H
#define SENDGRIDMSG_H

#include <string>
#include <vector>
#include <map>
#include "sendgrid.h"

namespace SendGrid
{
    class SendGridMessage
    {
    public:
        SendGridMessage();

        void setFrom(EmailAddress *email);
        void setReplyTo(EmailAddress *email);

        void setSubject(string subject)
        {
            this->subject = subject;
        }

        void AddContent(string mimeType, string text)
        {
            if (!contents)
                contents = new vector<Content>();

            Content content(mimeType, text);
            this->contents->push_back(content);
        }

        void addContents(vector<Content> contents)
        {
            if (!this->contents)
                this->contents = new vector<Content>();

            this->contents->insert(this->contents->end(), contents.begin(), contents.end());
        }

        void addAttachment(string filename, string content, string type = NULL, string disposition = NULL, string contentId = NULL)
        {
            if (!attachments)
                attachments = new vector<Attachment>;
            Attachment attachment(content, type, filename, disposition, contentId);
            this->attachments->push_back(attachment);
        }

        void addAttachments(vector<Attachment> attachments)
        {
            if (!this->attachments)
                this->attachments = new vector<Attachment>;

            this->attachments->insert(this->attachments->end(), attachments.begin(), attachments.end());
        }

        void addPersonalization(Personalization personalization)
        {
            if (!personalizations)
                personalizations = new vector<Personalization>;

            this->personalizations->push_back(personalization);
        }

        void setTemplateId(string templateID)
        {
            this->templateId = templateID;
        }

        void addSection(string key, string value)
        {
            if (!sections)
                sections = new map<string, string>;

            this->sections->insert(make_pair(key, value));
        }

        void addSections(map<string, string> sections)
        {
            if (!this->sections)
                this->sections = new map<string, string>;

            this->sections->insert(sections.begin(), sections.end());
        }

        void addHeader(string key, string value)
        {
            if (!headers)
                headers = new map<string, string>;

            this->headers->insert(make_pair(key, value));
        }
        void addHeaders(map<string, string> headers)
        {
            if (!this->headers)
                this->headers = new map<string, string>;

            this->headers->insert(headers.begin(), headers.end());
        }
        void addCategory(string category)
        {
            if (!this->categories)
                this->categories = new vector<string>;

            this->categories->push_back(category);
        }

        void addCategories(vector<string> categories)
        {
            if (!this->categories)
                this->categories = new vector<string>;

            this->categories->insert(this->categories->end(), categories.begin(), categories.end());
        }
        void addCustomArg(string key, string value)
        {
            if (!this->customArgs)
                this->customArgs = new map<string, string>;

            this->customArgs->insert(make_pair(key, value));
        }

        void addCustomArgs(map<string, string> customArgs)
        {
            if (!this->customArgs)
                this->customArgs = new map<string, string>;

            this->customArgs->insert(customArgs.begin(), customArgs.end());
        }

        void setSendAt(long sendAt)
        {
            this->sendAt = sendAt;
        }

        void setBatchId(string batchId)
        {
            this->batchId = batchId;
        }
        void setAsm(int groupID, vector<int> groupsToDisplay)
        {
            if (this->_asm)
                delete this->_asm;

            this->_asm = new ASM{groupID, groupsToDisplay};
        }

        void setIpPoolName(string ipPoolName)
        {
            this->ipPoolName = ipPoolName;
        }

        void setBccSetting(bool enable, string email)
        {
            if (!this->mailSettings)
                this->mailSettings = new MailSettings();

            if (this->mailSettings->bccSettings)
                delete this->mailSettings->bccSettings;

            this->mailSettings->bccSettings = new BCCSettings{enable, email};
        }

        void setFooterSetting(bool enable, string html = NULL, string text = NULL)
        {
            if (!this->mailSettings)
                this->mailSettings = new MailSettings();

            if (this->mailSettings->footerSettings)
                delete this->mailSettings->footerSettings;

            this->mailSettings->footerSettings = new FooterSettings{enable, html, text};
        }

        void setSandBoxMode(bool enable)
        {
            if (!this->mailSettings)
                this->mailSettings = new MailSettings();
            if (this->mailSettings->sandboxMode)
                delete this->mailSettings->sandboxMode;

            this->mailSettings->sandboxMode = new SandboxMode{enable};
        }

        string toString()
        {
            if (!this->plainTextContent.empty() || !this->htmlContent.empty())
            {
                if (!this->plainTextContent.empty())
                    this->contents->push_back({SendGridMimeTypeText, this->plainTextContent});

                if (!this->htmlContent.empty())
                    this->contents->push_back({SendGridMimeTypeHTML, this->htmlContent});

                this->plainTextContent = "";
                this->htmlContent = "";
            }

            if (this->contents)
            {
                // MimeType.Text > MimeType.Html > Everything Else
                for (size_t i = 0; i < this->contents->size(); i++)
                {
                    if ((*this->contents)[i].type == SendGridMimeTypeHTML)
                    {
                        auto tempContent = (*this->contents)[i];
                        this->contents->erase(this->contents->begin() + i);
                        this->contents->insert(this->contents->begin(), tempContent);
                    }

                    if ((*this->contents)[i].type == SendGridMimeTypeText)
                    {
                        auto tempContent = (*this->contents)[i];
                        this->contents->erase(this->contents->begin() + i);
                        this->contents->insert(this->contents->begin(), tempContent);
                    }
                }
            }

            neb::CJsonObject oJson;
            if (from)
                oJson.Add("from", from->toJson());
            if (!subject.empty())
                oJson.Add("subject", subject);
            if (personalizations)
            {
                oJson.AddEmptySubArray("personalizations");
                for (size_t i = 0; i < personalizations->size(); i++)
                {
                    oJson["personalizations"].Add((*personalizations)[i].toJson());
                }
            }
            if (contents)
            {
                oJson.AddEmptySubArray("content");
                vector<CJsonObject> vec = listToJson2(*contents);
                for (CJsonObject obj : vec)
                {
                    oJson["content"].Add(obj);
                }
            }

            if (attachments)
            {
                oJson.AddEmptySubArray("attachments");
                vector<CJsonObject> vec = listToJson2(*attachments);
                for (CJsonObject obj : vec)
                {
                    oJson["attachments"].Add(obj);
                }
            }

            if (!templateId.empty())
                oJson.Add("template_id", templateId);

            if (headers)
                oJson.Add("headers", hashToJson(*headers));
            if (sections)
                oJson.Add("sections", hashToJson(*sections));
            if (categories)
            {
                oJson.AddEmptySubArray("categories");
                vector<CJsonObject> vec = listToJson(*categories);
                for (CJsonObject obj : vec)
                {
                    oJson["categories"].Add(obj);
                }
            }

            if (customArgs)
                oJson.Add("custom_args", hashToJson(*customArgs));
            if (sendAt > 0)
                oJson.Add("send_at", sendAt);
            if (_asm)
                oJson.Add("asm", _asm->toJson());
            if (!batchId.empty())
                oJson.Add("batch_id", batchId);
            if (!ipPoolName.empty())
                oJson.Add("ip_pool_name", ipPoolName);
            if (mailSettings)
                oJson.Add("mail_settings", mailSettings->toJson());
            if (replyTo)
                oJson.Add("reply_to", replyTo->toJson());

            return oJson.ToString();
        }

    private:
        EmailAddress *from = NULL;
        string subject;
        vector<Personalization> *personalizations = NULL;
        vector<Content> *contents = NULL;
        string plainTextContent;
        string htmlContent;
        vector<Attachment> *attachments = NULL;
        string templateId;
        map<string, string> *headers = NULL;
        map<string, string> *sections = NULL;
        vector<string> *categories = NULL;
        map<string, string> *customArgs = NULL;
        uint32 sendAt = 0;
        ASM *_asm = NULL;
        string batchId;
        string ipPoolName;
        MailSettings *mailSettings = NULL;
        EmailAddress *replyTo = NULL;
    };
}
#endif